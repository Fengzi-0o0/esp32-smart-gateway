#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "mqtt_client.h"
#include "gpio_control.h"
#include "timer_engine.h"
#include "logic_engine.h"
#include "i2c_engine.h"
#include "onewire_engine.h"
#include "rtc_engine.h"
#include "rand_engine.h"
#include "script_engine.h"
#include "uart_engine.h"
#include "spi_engine.h"
#include "touch_engine.h"
#include <esp_task_wdt.h>
#include "data_engine.h"
#include "dual_channel.h"



const char *OTA_VERIFY_TAG = "阶段完成";


WebServer server(80);
DeviceConfig config;
volatile bool otaInProgress = false;

// ================================================================
//  看门狗配置
// ================================================================
#define WDT_TIMEOUT_SEC  30

static bool wdtEnabled = false;

void wdtInit() {
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms   = WDT_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,       // 不监控 idle 任务
        .trigger_panic  = true     // 超时触发 panic 重启
    };

    esp_err_t err = esp_task_wdt_reconfigure(&wdt_config);
    if (err != ESP_OK) {
        err = esp_task_wdt_init(&wdt_config);
    }

    if (err == ESP_OK) {
        Serial.printf("[WDT] Initialized, timeout=%ds, panic=true\n", WDT_TIMEOUT_SEC);
    } else {
        Serial.printf("[WDT] Init failed: %d\n", err);
        return;
    }

    err = esp_task_wdt_add(NULL);
    if (err == ESP_OK) {
        wdtEnabled = true;
        Serial.println("[WDT] Main loop task added");
    } else {
        Serial.printf("[WDT] Add task failed: %d\n", err);
    }
}


void wdtFeed() {
    if (wdtEnabled) {
        esp_task_wdt_reset();
    }
}

// OTA 期间暂停看门狗
void wdtPause() {
    if (wdtEnabled) {
        esp_task_wdt_delete(NULL);
        wdtEnabled = false;
        Serial.println("[WDT] Paused for OTA");
    }
}

// OTA 结束恢复看门狗
void wdtResume() {
    if (!wdtEnabled) {
        esp_err_t err = esp_task_wdt_add(NULL);
        if (err == ESP_OK) {
            wdtEnabled = true;
            Serial.println("[WDT] Resumed after OTA");
        }
    }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==============================");
  Serial.printf(" ESP32-S3 Smart Gateway 1.3.0 by %s\n", OTA_VERIFY_TAG);
  Serial.printf(" Device ID: %s\n", getDeviceId().c_str());
  Serial.printf(" Firmware: v%s\n", FIRMWARE_VERSION);
  Serial.println("==============================\n");

  // ========== 看门狗初始化（在所有引擎之前） ==========
  wdtInit();

  config.load();
  // 恢复持久化定时器
  for (auto &te : config.timers) {
    if (!te.persistent) continue;
    TimerTask t;
    t.id = te.id;
    t.type = te.type;
    t.interval = te.interval;
    t.count = te.count;
    t.enabled = te.enabled;
    t.commandsJson = te.commandsJson;
    TimerEngine::add(t);
  }

  // 恢复持久化逻辑规则
  for (auto &lre : config.logicRules) {
    if (!lre.persistent) continue;
    LogicRule r;
    r.id = lre.id;
    r.operator_ = lre.operator_;
    r.enabled = lre.enabled;
    r.cooldown = lre.cooldown;
    r.actionsJson = lre.actionsJson;
    for (auto &ce : lre.conditions) {
      SimpleCondition sc;
      sc.source = ce.source;
      sc.pin = ce.pin;
      sc.op = ce.op;
      sc.value = ce.value;
      r.conditions.push_back(sc);
    }
    LogicEngine::add(r);
  }

  GpioControl::init();
  TimerEngine::init();
  LogicEngine::init();
  I2C::init();
  OneWire::init();
  RTC::init();
  Rand::init();
  UART::init();
  SPIBus::init();
  Touch::init();

  WifiManager::init();
  WebServerManager::init();
  MqttClient::init();

  // 恢复定时器到引擎
  for (auto &te : config.timers) {
    TimerTask t;
    t.id = te.id;
    t.type = te.type;
    t.interval = te.interval;
    t.count = te.count;
    t.enabled = te.enabled;
    t.commandsJson = te.commandsJson;
    TimerEngine::add(t);
  }
  Serial.printf("[MAIN] Restored %d timers from NVS\n", config.timers.size());

  // 恢复逻辑规则到引擎
  for (auto &lre : config.logicRules) {
    LogicRule r;
    r.id = lre.id;
    r.operator_ = lre.operator_;
    r.enabled = lre.enabled;
    r.cooldown = lre.cooldown;
    r.actionsJson = lre.actionsJson;
    for (auto &ce : lre.conditions) {
      SimpleCondition sc;
      sc.source = ce.source;
      sc.pin = ce.pin;
      sc.op = ce.op;
      sc.value = ce.value;
      r.conditions.push_back(sc);
    }
    LogicEngine::add(r);
  }
  Serial.printf("[MAIN] Restored %d logic rules from NVS\n", config.logicRules.size());
  Serial.printf("[MAIN] I2C configs: %d, 1-Wire configs: %d\n",
                config.i2cConfigs.size(), config.owConfigs.size());
  ScriptEngine::init();
  DataEngine::init();
  DualChannel::init(); 

  Serial.println("[MAIN] System ready!\n");
}

void loop() {
  wdtFeed();  // 喂狗

  // OTA 期间暂停看门狗，避免 Flash 写入耗时触发重启
  if (otaInProgress) {
    wdtPause();
  } else {
    wdtResume();
  }

  WifiManager::loop();
  WebServerManager::loop();
  MqttClient::loop();
  TimerEngine::loop();
  LogicEngine::loop();
  ScriptEngine::loop();
  UART::loop();
  SPIBus::loop();
  Touch::loop();
  RTC::loop();
  DataEngine::loop();
  DualChannel::loop(); 

}
