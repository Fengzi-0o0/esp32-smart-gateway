#include "mqtt_client.h"
#include "config.h"
#include "wifi_manager.h"
#include "gpio_control.h"
#include "timer_engine.h"
#include "logic_engine.h"
#include "i2c_engine.h"
#include "onewire_engine.h"
#include "rtc_engine.h"
#include "rand_engine.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Update.h>
#include <ESP32Servo.h>
#include "script_engine.h"
#include "pin_caps.h"
#include "uart_engine.h"
#include "spi_engine.h"
#include "touch_engine.h"
#include "data_engine.h"
#include "msg_dedup.h"
#include "dual_channel.h"
#include "encoder_engine.h"
#include "display_engine.h"  // �?新增






static WiFiClient wifiClient;
static WiFiClientSecure sslClient;
static PubSubClient mqttClient(wifiClient);

// ===== WS 结果转发 =====
typedef void (*WsSendFunc)(uint8_t, const char *);
static WsSendFunc _wsSend = nullptr;
static uint8_t _wsClient = 0xFF;

void setResultSink(uint8_t clientNum, WsSendFunc fn) {
  _wsClient = clientNum;
  _wsSend = fn;
}

void clearResultSink() {
  _wsClient = 0xFF;
  _wsSend = nullptr;
}

static void publishResult(const String &payload) {
  if (mqttClient.connected() && config.pubTopics.size() > 0)
    mqttClient.publish(config.pubTopics[0].topic.c_str(), payload.c_str());
  if (_wsClient != 0xFF && _wsSend)
    _wsSend(_wsClient, payload.c_str());
}

bool MqttClient::publish(const String &json) {
  if (!mqttClient.connected()) return false;

  // ══════════════════════════════════════════════════════════�?
  // ★★�?新增：注�?_from 字段，让接收方能识别消息来源 ★★�?
  // ══════════════════════════════════════════════════════════�?
  String finalJson = json;
  {
    JsonDocument doc;
    if (!deserializeJson(doc, json)) {
      doc["_from"] = getDeviceId();
      serializeJson(doc, finalJson);
    }
  }
  // ══════════════════════════════════════════════════════════�?
  // ★★�?新增结束 ★★�?
  // ══════════════════════════════════════════════════════════�?

  bool anyOk = false;
  // 发到 subTopics（所有设备都订阅的共享命令通道�?
  for (size_t i = 0; i < config.subTopics.size(); i++) {
    if (mqttClient.publish(config.subTopics[i].topic.c_str(), finalJson.c_str()))
      anyOk = true;
  }
  return anyOk;
}





static bool connected = false;
static bool sslPreConnected = false;



// 非阻塞重连状态机
enum MqttState { MQTT_IDLE,
                 MQTT_RESOLVING,
                 MQTT_CONNECTING,
                 MQTT_LINKED };
static MqttState mqttState = MQTT_IDLE;
static unsigned long mqttStateTime = 0;
static unsigned long mqttReconnectBackoff = 2000;
static const unsigned long MQTT_MAX_BACKOFF = 30000;
static IPAddress resolvedIP;
static String pendingClientId;

// MQTT OTA
static bool mqttOtaActive = false;
static size_t otaTotalSize = 0;
static size_t otaChunks = 0;
static uint32_t otaChunkCount = 0;
static size_t otaWritten = 0;
static unsigned long otaLastChunk = 0;



// PubSubClient 错误码转字符�?
static const char *mqttErrorString(int rc) {
  switch (rc) {
    case -4: return "CONNECTION_TIMEOUT";
    case -3: return "CONNECTION_LOST";
    case -2: return "CONNECT_FAILED";
    case -1: return "DISCONNECTED";
    case 0: return "CONNECTED";
    case 1: return "BAD_PROTOCOL";
    case 2: return "BAD_CLIENT_ID";
    case 3: return "UNAVAILABLE";
    case 4: return "BAD_CREDENTIALS";
    case 5: return "UNAUTHORIZED";
    default: return "UNKNOWN";
  }
}

// 舵机
#define MAX_SERVOS 8
struct ServoEntry {
  Servo servo;
  int pin;
  int angle;
  bool attached;
};
static ServoEntry servoList[MAX_SERVOS];
static int servoCount = 0;

static ServoEntry *findOrAddServo(int pin) {
  for (int i = 0; i < servoCount; i++) {
    if (servoList[i].pin == pin) return &servoList[i];
  }
  if (servoCount < MAX_SERVOS) {
    ServoEntry &se = servoList[servoCount];
    se.pin = pin;
    se.angle = 0;
    se.attached = false;
    servoCount++;
    return &se;
  }
  return nullptr;
}

struct ModuleEntry {
  int pin;
  String moduleType;   // "ds18b20", "dht11", "dht22"
  String label;
  int interval;
  bool enabled;
};
static std::vector<ModuleEntry> moduleRegistry;

static ModuleEntry *findModule(int pin) {
  for (auto &m : moduleRegistry) {
    if (m.pin == pin) return &m;
  }
  return nullptr;
}

// 批量上报
static unsigned long lastBatchReport = 0;
static const char *LWT_TOPIC = "esp32/status";
static String lwtOnlineMsg;
static String lwtOfflineMsg;

static void publishBatchStatus() {
  if (!mqttClient.connected() || config.pubTopics.size() == 0) return;

  JsonDocument doc;
  doc["type"] = "batch_status";
  doc["deviceId"] = getDeviceId();
  doc["mac"] = WiFi.macAddress();  //设备序列�?
  doc["timestamp"] = millis();
  doc["heapFree"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["otaVerify"] = OTA_VERIFY_TAG;  // 新增

  JsonArray snrs = doc["sensors"].to<JsonArray>();
  for (auto &s : config.sensors) {
    if (!s.enabled) continue;
    JsonObject o = snrs.add<JsonObject>();
    o["pin"] = s.pin;
    o["label"] = s.label;
    o["raw"] = analogRead(s.pin);
  }

  JsonArray ins = doc["inputs"].to<JsonArray>();
  for (auto &t : config.inputs) {
    if (!t.enabled) continue;
    JsonObject o = ins.add<JsonObject>();
    o["pin"] = t.pin;
    o["label"] = t.label;
    o["value"] = digitalRead(t.pin);
  }

  String payload;
  serializeJson(doc, payload);
  publishResult(payload);
  Serial.println("[BATCH] Status published");
}

// 串口缓冲
static String serialBuffer = "";
static MsgDedup cmdDedup;

// 前向声明
static void handleCommand(JsonDocument &doc, bool fromMqtt = false);

static void handleSensorCommand(JsonDocument &doc);
static void handleInputCommand(JsonDocument &doc);
static void handleTimerCommand(JsonDocument &doc);
static void handleLogicCommand(JsonDocument &doc);

static void executeCommandImpl(JsonDocument &doc) {
  handleCommand(doc);
}

void executeCommand(JsonDocument &doc) __attribute__((weak));
void executeCommand(JsonDocument &doc) {
  executeCommandImpl(doc);
}

// 传感器指令处�?
static void handleSensorCommand(JsonDocument &doc) {
  String action = doc["action"].as<String>();

  if (action == "add") {
    SensorTask s;
    s.pin = doc["pin"] | -1;
    s.interval = doc["interval"] | 5000;
    s.type = doc["type"] | 0;
    s.enabled = true;
    s.label = doc["label"].as<String>();
    bool persist = doc["persistent"] | false;  // 新增

    if (s.pin >= 0 && !isValidExternalPin(s.pin)) {
      MqttClient::publishLog("warn", "SENSOR", "REJECTED: pin=%d not in external pinout", s.pin);
      return;
    }

    s.interval = doc["interval"] | 5000;

    if (s.pin >= 0 && config.sensors.size() < MAX_SENSORS) {
      config.sensors.push_back(s);
      config.sensors.back().persistent = persist;

      if (persist) {
        config.markDirty();
        Serial.printf("[SENSOR] Added pin=%d interval=%d type=%d label=%s (persistent)\n",
                      s.pin, s.interval, s.type, s.label.c_str());
      } else {
        Serial.printf("[SENSOR] Added pin=%d interval=%d type=%d label=%s (non-persistent)\n",
                      s.pin, s.interval, s.type, s.label.c_str());
      }
    }

  } else if (action == "remove") {
    int pin = doc["pin"] | -1;
    for (auto it = config.sensors.begin(); it != config.sensors.end(); ++it) {
      if (it->pin == pin) {
        bool wasPersistent = it->persistent;
        config.sensors.erase(it);
        if (wasPersistent) config.markDirty();  // 只有原本持久化的才需要更新NVS
        Serial.printf("[SENSOR] Removed pin=%d\n", pin);
        break;
      }
    }

  } else if (action == "enable") {
    int pin = doc["pin"] | -1;
    bool en = doc["enabled"] | true;
    for (auto &s : config.sensors) {
      if (s.pin == pin) {
        s.enabled = en;
        if (s.persistent) config.markDirty();  // 只有持久化的才需要更新NVS
        Serial.printf("[SENSOR] Pin %d %s\n", pin, en ? "enabled" : "disabled");
        break;
      }
    }

  } else if (action == "interval") {
    int pin = doc["pin"] | -1;
    int interval = doc["interval"] | 5000;
    for (auto &s : config.sensors) {
      if (s.pin == pin) {
        s.interval = interval;
        if (s.persistent) config.markDirty();  // 只有持久化的才需要更新NVS
        Serial.printf("[SENSOR] Pin %d interval=%d\n", pin, interval);
        break;
      }
    }

  } else if (action == "read") {
    int pin = doc["pin"] | -1;
    if (pin >= 0) {
      int raw = analogRead(pin);
      Serial.printf("[SENSOR] Read pin=%d raw=%d\n", pin, raw);
      JsonDocument resp;
      resp["type"] = "sensor";
      resp["deviceId"] = getDeviceId();
      resp["pin"] = pin;
      resp["raw"] = raw;
      for (auto &s : config.sensors) {
        if (s.pin == pin) {
          resp["label"] = s.label;
          break;
        }
      }
      if (doc.containsKey("result_var")) {
        String varName = doc["result_var"].as<String>();
        if (varName.length() > 0)
          ScriptEngine::setVar(varName, (float)raw, false);
      }

      String p;
      serializeJson(resp, p);
      if (mqttClient.connected() && config.pubTopics.size() > 0) {
        publishResult(p);
      }
    }

  } else if (action == "list") {
    JsonDocument resp;
    resp["type"] = "sensor_list";
    resp["deviceId"] = getDeviceId();
    JsonArray arr = resp["sensors"].to<JsonArray>();
    for (auto &s : config.sensors) {
      JsonObject o = arr.add<JsonObject>();
      o["pin"] = s.pin;
      o["interval"] = s.interval;
      o["type"] = s.type;
      o["enabled"] = s.enabled;
      o["label"] = s.label;
      o["persistent"] = s.persistent;  // 新增：列表中也显示持久化状�?
    }
    String p;
    serializeJson(resp, p);
    publishResult(p);
    Serial.println("[SENSOR] List published");



  } else {
    Serial.printf("[SENSOR] Unknown: %s\n", action.c_str());
  }
}


// 数字输入指令处理
static void handleInputCommand(JsonDocument &doc) {
  String action = doc["action"].as<String>();

  if (action == "add") {
    InputTask t;
    t.pin = doc["pin"] | -1;
    t.mode = doc["mode"] | INPUT_PULLUP;
    t.debounceMs = doc["debounce"] | 50;
    t.enabled = true;
    t.label = doc["label"].as<String>();
    t.lastValue = -1;
    t.lastChange = 0;
    bool persist = doc["persistent"] | false;  // 新增

    if (t.pin >= 0 && !isValidExternalPin(t.pin)) {
      MqttClient::publishLog("warn", "INPUT", "REJECTED: pin=%d not in external pinout", t.pin);
      return;
    }

    t.mode = doc["mode"] | INPUT_PULLUP;

    if (t.pin >= 0 && config.inputs.size() < MAX_INPUTS) {
      int cm = GpioControl::getPinMode(t.pin);
      if (cm == OUTPUT) {
        MqttClient::publishLog("warn", "INPUT", "REJECTED: pin=%d is OUTPUT", t.pin);
      } else {
        pinMode(t.pin, t.mode);
        t.lastValue = digitalRead(t.pin);
        t.persistent = persist;
        config.inputs.push_back(t);

        if (persist) {
          config.markDirty();
          Serial.printf("[INPUT] Added pin=%d mode=%d lbl=%s (persistent)\n",
                        t.pin, t.mode, t.label.c_str());
        } else {
          Serial.printf("[INPUT] Added pin=%d mode=%d lbl=%s (non-persistent)\n",
                        t.pin, t.mode, t.label.c_str());
        }
      }
    }

  } else if (action == "remove") {
    int pin = doc["pin"] | -1;
    for (auto it = config.inputs.begin(); it != config.inputs.end(); ++it) {
      if (it->pin == pin) {
        bool wasPersistent = it->persistent;
        config.inputs.erase(it);
        if (wasPersistent) config.markDirty();  // 只有原本持久化的才需要更新NVS
        Serial.printf("[INPUT] Removed pin=%d\n", pin);
        break;
      }
    }

  } else if (action == "enable") {
    int pin = doc["pin"] | -1;
    bool en = doc["enabled"] | true;
    for (auto &t : config.inputs) {
      if (t.pin == pin) {
        t.enabled = en;
        if (t.persistent) config.markDirty();  // 只有持久化的才需要更新NVS
        Serial.printf("[INPUT] Pin %d %s\n", pin, en ? "enabled" : "disabled");
        break;
      }
    }

  } else if (action == "list") {
    JsonDocument resp;
    resp["type"] = "input_list";
    resp["deviceId"] = getDeviceId();
    JsonArray arr = resp["inputs"].to<JsonArray>();
    for (auto &t : config.inputs) {
      JsonObject o = arr.add<JsonObject>();
      o["pin"] = t.pin;
      o["mode"] = t.mode;
      o["enabled"] = t.enabled;
      o["label"] = t.label;
      o["lastValue"] = t.lastValue;
      o["persistent"] = t.persistent;  // 新增：列表中也显示持久化状�?
    }
    String p;
    serializeJson(resp, p);
    publishResult(p);
    Serial.println("[INPUT] List published");



  } else {
    Serial.printf("[INPUT] Unknown: %s\n", action.c_str());
  }
}


// 定时器指令处�?
static void handleTimerCommand(JsonDocument &doc) {
  String action = doc["action"].as<String>();

  if (action == "add") {
    TimerTask t;
    t.id = doc["id"].as<String>();
    t.type = doc["type"].as<String>();
    t.interval = doc["interval"] | 5000;
    t.count = doc["count"] | -1;
    t.enabled = doc["enabled"] | true;
    t.executed = 0;
    t.duration = doc["duration"] | 0;
    t.autoDelete = doc["autoDelete"] | false;
    bool persist = doc["persistent"] | false;  // �?必须在这里声�?

    if (doc.containsKey("commands")) {
      JsonDocument cmdsDoc;
      if (!deserializeJson(cmdsDoc, doc["commands"])) {
        if (cmdsDoc.is<JsonArray>()) {
          for (JsonObject c : cmdsDoc.as<JsonArray>()) {
            if (c.containsKey("pin")) {
              int p = c["pin"].as<int>();
              if (!isValidExternalPin(p)) {
                Serial.printf("[TIMER] REJECTED: pin=%d not in external pinout\n", p);
                return;
              }
            }
            if (c.containsKey("rPin")) {
              int p = c["rPin"].as<int>();
              if (!isValidExternalPin(p)) {
                Serial.printf("[TIMER] REJECTED: rPin=%d\n", p);
                return;
              }
            }
            if (c.containsKey("gPin")) {
              int p = c["gPin"].as<int>();
              if (!isValidExternalPin(p)) {
                Serial.printf("[TIMER] REJECTED: gPin=%d\n", p);
                return;
              }
            }
            if (c.containsKey("bPin")) {
              int p = c["bPin"].as<int>();
              if (!isValidExternalPin(p)) {
                Serial.printf("[TIMER] REJECTED: bPin=%d\n", p);
                return;
              }
            }
          }
        }
      }
      String cmdsStr;
      serializeJson(doc["commands"], cmdsStr);
      t.commandsJson = cmdsStr;
    } else {
      t.commandsJson = "[]";
    }

    TimerEngine::add(t);

    if (persist) {
      bool found = false;
      for (auto &te : config.timers) {
        if (te.id == t.id) {
          te.type = t.type;
          te.interval = t.interval;
          te.count = t.count;
          te.enabled = t.enabled;
          te.commandsJson = t.commandsJson;
          te.persistent = true;
          found = true;
          break;
        }
      }
      if (!found && config.timers.size() < MAX_TIMERS) {
        TimerEntry te;
        te.id = t.id;
        te.type = t.type;
        te.interval = t.interval;
        te.count = t.count;
        te.enabled = t.enabled;
        te.commandsJson = t.commandsJson;
        te.persistent = true;
        config.timers.push_back(te);
      }
      config.markDirty();
    } else {
      Serial.printf("[TIMER] '%s' is non-persistent (not saved to NVS)\n", t.id.c_str());
    }

  } else if (action == "remove") {
    String id = doc["id"].as<String>();
    TimerEngine::remove(id);
    for (auto it = config.timers.begin(); it != config.timers.end(); ++it) {
      if (it->id == id) {
        config.timers.erase(it);
        break;
      }
    }
    config.markDirty();

  } else if (action == "enable") {
    String id = doc["id"].as<String>();
    bool en = doc["enabled"] | true;
    TimerEngine::enable(id, en);
    for (auto &te : config.timers) {
      if (te.id == id) {
        te.enabled = en;
        break;
      }
    }
    config.markDirty();

  } else if (action == "reset") {
    String id = doc["id"].as<String>();
    TimerEngine::reset(id);

  } else if (action == "list") {
    String json = TimerEngine::toJson();
    publishResult(json);
    Serial.println("[TIMER] List published");



  } else {
    Serial.printf("[TIMER] Unknown action: %s\n", action.c_str());
  }
}


// 逻辑规则指令处理
static void handleLogicCommand(JsonDocument &doc) {
  String action = doc["action"].as<String>();

  if (action == "add") {
    LogicRule r;
    r.id = doc["id"].as<String>();
    r.operator_ = doc["operator"] | String("and");
    r.enabled = doc["enabled"] | true;
    r.cooldown = doc["cooldown"] | 1000;
    bool persist = doc["persistent"] | false;  // 新增：默认true兼容旧用�?

    if (doc.containsKey("conditions")) {
      for (JsonObject c : doc["conditions"].as<JsonArray>()) {
        SimpleCondition sc;
        sc.source = c["source"].as<String>();
        sc.pin = c["pin"] | -1;

        if (sc.pin >= 0 && !isValidExternalPin(sc.pin)) {
          Serial.printf("[LOGIC] REJECTED: condition pin=%d not in external pinout\n", sc.pin);
          return;
        }

        sc.op = c["op"].as<String>();
        sc.value = c["value"] | 0.0f;
        sc.source_var = c["name"] | String("");  // 新增
        r.conditions.push_back(sc);
      }
    }

    if (doc.containsKey("actions")) {
      // 验证 actions 中的引脚
      JsonDocument actDoc;
      if (!deserializeJson(actDoc, doc["actions"])) {
        if (actDoc.is<JsonArray>()) {
          for (JsonObject a : actDoc.as<JsonArray>()) {
            if (a.containsKey("pin")) {
              int p = a["pin"].as<int>();
              if (!isValidExternalPin(p)) {
                Serial.printf("[LOGIC] REJECTED: action pin=%d not in external pinout\n", p);
                return;
              }
            }
          }
        }
      }
      String actsStr;
      serializeJson(doc["actions"], actsStr);
      r.actionsJson = actsStr;
    } else {
      r.actionsJson = "[]";
    }

    LogicEngine::add(r);  // 运行时总是加入

    // 只有 persistent=true 才写入NVS
    if (persist) {
      bool found = false;
      for (auto &lre : config.logicRules) {
        if (lre.id == r.id) {
          lre.operator_ = r.operator_;
          lre.enabled = r.enabled;
          lre.cooldown = r.cooldown;
          lre.actionsJson = r.actionsJson;
          lre.persistent = true;
          lre.conditions.clear();
          for (auto &sc : r.conditions) {
            ConditionEntry ce;
            ce.source = sc.source;
            ce.pin = sc.pin;
            ce.op = sc.op;
            ce.value = sc.value;
            lre.conditions.push_back(ce);
          }
          found = true;
          break;
        }
      }
      if (!found && config.logicRules.size() < MAX_LOGIC_RULES) {
        LogicRuleEntry lre;
        lre.id = r.id;
        lre.operator_ = r.operator_;
        lre.enabled = r.enabled;
        lre.cooldown = r.cooldown;
        lre.actionsJson = r.actionsJson;
        lre.persistent = true;
        for (auto &sc : r.conditions) {
          ConditionEntry ce;
          ce.source = sc.source;
          ce.pin = sc.pin;
          ce.op = sc.op;
          ce.value = sc.value;
          lre.conditions.push_back(ce);
        }
        config.logicRules.push_back(lre);
      }
      config.markDirty();
    } else {
      Serial.printf("[LOGIC] Rule '%s' is non-persistent (not saved to NVS)\n", r.id.c_str());
    }
  } else if (action == "remove") {
    String id = doc["id"].as<String>();
    LogicEngine::remove(id);
    for (auto it = config.logicRules.begin(); it != config.logicRules.end(); ++it) {
      if (it->id == id) {
        config.logicRules.erase(it);
        break;
      }
    }
    config.markDirty();
  } else if (action == "enable") {
    String id = doc["id"].as<String>();
    bool en = doc["enabled"] | true;
    LogicEngine::enable(id, en);
    for (auto &lre : config.logicRules) {
      if (lre.id == id) {
        lre.enabled = en;
        break;
      }
    }
    config.markDirty();
  } else if (action == "list") {
    String json = LogicEngine::toJson();
    publishResult(json);
    Serial.println("[LOGIC] List published");


  } else {
    Serial.printf("[LOGIC] Unknown action: %s\n", action.c_str());
  }
}

// 主命令分�?
static void handleCommand(JsonDocument &doc, bool fromMqtt) {


  // ===== _mid 去重 =====
  {
    uint16_t mid = MsgDedup::parseMid(doc);
    if (cmdDedup.isDuplicate(mid)) {
      Serial.printf("[CMD] Dup _mid=0x%04X, skipped\n", mid);
      return;
    }
    cmdDedup.record(mid);
  }

  if (doc.containsKey("target")) {
    String t = doc["target"].as<String>();
    if (t.length() > 0 && t != "all") {
      String myId = getDeviceId();
      String myName = config.deviceName;
      bool matchId = (t == myId);
      bool matchName = (myName.length() > 0 && t == myName);
      if (!matchId && !matchName) {
        String cmdCheck = doc["cmd"].as<String>();
        if (cmdCheck == "forward") {
        } else {
          int hops = doc["_hops"] | 0;
          if (hops >= DC_MAX_HOPS) {
            MqttClient::publishLog("warn", "CMD", "Hop limit (%d), drop forward to %s", hops, t.c_str());
            return;
          }
          doc["_hops"] = hops + 1;
          if (!doc.containsKey("_mid") || doc["_mid"].as<String>().length() == 0) {
            char midBuf[8];
            snprintf(midBuf, sizeof(midBuf), "%04X", (uint16_t)(esp_random() & 0xFFFF));
            doc["_mid"] = midBuf;
          }
          String json;
          serializeJson(doc, json);
          DualChannel::sendToTarget(t, json);
          return;
        }
      }
    }
  }
  String cmd = doc["cmd"].as<String>();


  if (cmd == "set") {

    int pin = doc["pin"] | -1;
    if (pin >= 0 && !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "GPIO", "REJECTED: pin=%d not in external pinout", pin);
      return;
    }
    int value = 0;
    if (doc["value"].is<int>()) {
      value = doc["value"].as<int>();
    } else if (doc["value"].is<float>()) {
      value = (int)doc["value"].as<float>();
    } else if (doc["value"].is<String>()) {
      value = doc["value"].as<String>().toInt();
    }
    if (pin >= 0) {
      GpioControl::digitalSet(pin, value);
      Serial.printf("[GPIO] SET pin=%d value=%d\n", pin, value);
      GpioControl::publishStatus();
    }
  } else if (cmd == "toggle") {
    int pin = doc["pin"] | -1;
    if (pin >= 0 && !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "GPIO", "REJECTED: pin=%d not in external pinout", pin);
      return;
    }
    if (pin >= 0) {
      GpioControl::digitalToggle(pin);
      Serial.printf("[GPIO] TOGGLE pin=%d\n", pin);
      GpioControl::publishStatus();
    }
  } else if (cmd == "mode") {
    int pin = doc["pin"] | -1;
    if (pin >= 0 && !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "GPIO", "REJECTED: pin=%d not in external pinout", pin);
      return;
    }
    String modeStr = doc["mode"].as<String>();
    if (pin >= 0) {
      int m = OUTPUT;
      if (modeStr == "input") m = INPUT;
      else if (modeStr == "input_pullup") m = INPUT_PULLUP;
      GpioControl::setMode(pin, m);
      Serial.printf("[GPIO] MODE pin=%d mode=%s\n", pin, modeStr.c_str());
      GpioControl::publishStatus();
    }
  } else if (cmd == "pwm") {
    int pin = doc["pin"] | -1;
    if (pin >= 0 && !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "GPIO", "REJECTED: pin=%d not in external pinout", pin);
      return;
    }
    int value = 0;
    if (doc["value"].is<int>()) {
      value = doc["value"].as<int>();
    } else if (doc["value"].is<float>()) {
      value = (int)doc["value"].as<float>();
    } else if (doc["value"].is<String>()) {
      // 兼容 $V: 变量插值后变为字符串的情况
      value = doc["value"].as<String>().toInt();
    }
    if (pin >= 0) {
      GpioControl::analogSet(pin, value);
      Serial.printf("[GPIO] PWM pin=%d value=%d\n", pin, value);
      GpioControl::publishStatus();
    }
  }

  else if (cmd == "publish") {
    String topic = doc["topic"].as<String>();
    String payload;
    if (doc.containsKey("payload")) {
      if (doc["payload"].is<JsonObject>() || doc["payload"].is<JsonArray>()) {
        serializeJson(doc["payload"], payload);
      } else {
        payload = doc["payload"].as<String>();
      }
    }

    // ===== 变量插值："$V:varname" �?数字（去掉引号） =====
    {
      int pos;
      int loopGuard = 0;
      while ((pos = payload.indexOf("\"$V:")) >= 0 && loopGuard < 20) {
        loopGuard++;
        String varName = "";
        int nameStart = pos + 4;
        for (int i = nameStart; i < (int)payload.length(); i++) {
          char c = payload.charAt(i);
          if (isAlphaNumeric(c) || c == '_') varName += c;
          else break;
        }
        if (varName.length() > 0
            && nameStart + (int)varName.length() < (int)payload.length()
            && payload.charAt(nameStart + varName.length()) == '"') {
          float val = ScriptEngine::getVar(varName, 0);
            int nameEnd = nameStart + varName.length() + 1;
          payload = payload.substring(0, pos + 1)
                    + String((int)val)
                    + payload.substring(nameEnd - 1);

          Serial.printf("[PUBLISH] Interpolated $V:%s �?%d\n", varName.c_str(), (int)val);
        } else break;
      }
    }
    // ===== 插值结�?=====
    if (payload.startsWith("{")) {
      JsonDocument payloadDoc;
      if (!deserializeJson(payloadDoc, payload)) {
        payloadDoc["_from"] = getDeviceId();
        serializeJson(payloadDoc, payload);
      }
    }

    if (topic.length() > 0 && MqttClient::isConnected()) {
      MqttClient::publish(topic.c_str(), payload.c_str());
      Serial.printf("[MQTT] Publish to %s: %s\n", topic.c_str(), payload.c_str());
    }
  }

  // ══════════════════════════════════════════════════════════�?
  // ★★�?新增：forward 命令 �?跨设备转�?★★�?
  // ══════════════════════════════════════════════════════════�?
  // ══════════════════════════════════════════════════════════�?
  // ★★�?forward 命令 �?跨设备转�?★★�?
  // ══════════════════════════════════════════════════════════�?
  else if (cmd == "forward") {
    String target = doc["target"].as<String>();
    if (target.length() == 0) {
      Serial.println("[FWD] target required");
      return;
    }
    if (!doc.containsKey("payload")) {
      Serial.println("[FWD] payload required");
      return;
    }

    // 1. 序列�?payload 为字符串
    String payloadStr;
    serializeJson(doc["payload"], payloadStr);

    // 2. $V: 变量插�?
    {
      int pos;
      int guard = 0;
      while ((pos = payloadStr.indexOf("\"$V:")) >= 0 && guard < 20) {
        guard++;
        String varName = "";
        int nameStart = pos + 4;
        for (int i = nameStart; i < (int)payloadStr.length(); i++) {
          char c = payloadStr.charAt(i);
          if (isAlphaNumeric(c) || c == '_') varName += c;
          else break;
        }
        if (varName.length() > 0
            && nameStart + (int)varName.length() < (int)payloadStr.length()
            && payloadStr.charAt(nameStart + varName.length()) == '"') {
          float val = ScriptEngine::getVar(varName, 0);
          int nameEnd = nameStart + varName.length() + 1;
          payloadStr = payloadStr.substring(0, pos + 1)
                       + String((int)val)
                       + payloadStr.substring(nameEnd - 1);

          Serial.printf("[FWD] $V:%s �?%d\n", varName.c_str(), (int)val);
        } else break;
      }
    }

    // 3. 判断 target 是否是自�?
    String myId = getDeviceId();
    String myName = config.deviceName;
    bool isSelf = (target == myId) || (myName.length() > 0 && target == myName);

    if (isSelf) {
      // ★★�?target 是自己，直接本地执行 payload ★★�?
      JsonDocument payloadDoc;
      if (!deserializeJson(payloadDoc, payloadStr)) {
        Serial.printf("[FWD] Local exec: %s\n", payloadStr.c_str());
        executeCommand(payloadDoc);
      }
    } else {
      // ★★�?target 是其他设备，注入 target �?_from，通过 DualChannel 发�?★★�?
      JsonDocument payloadDoc;
      if (!deserializeJson(payloadDoc, payloadStr)) {
        payloadDoc["target"] = target;
        payloadDoc["_from"] = getDeviceId();
        serializeJson(payloadDoc, payloadStr);
      }
      bool ok = DualChannel::sendToTarget(target, payloadStr);
      Serial.printf("[FWD] �?%s (%s): %s\n",
                    target.c_str(), ok ? "OK" : "FAIL", payloadStr.c_str());
    }
  }
  // ══════════════════════════════════════════════════════════�?
  // ★★�?forward 结束 ★★�?
  // ══════════════════════════════════════════════════════════�?


  else if (cmd == "rgb") {
    int rPin = doc["rPin"] | -1;
    int gPin = doc["gPin"] | -1;
    int bPin = doc["bPin"] | -1;
    if ((rPin >= 0 && !isValidExternalPin(rPin)) || (gPin >= 0 && !isValidExternalPin(gPin)) || (bPin >= 0 && !isValidExternalPin(bPin))) {
      Serial.println("[GPIO] RGB: invalid pin in external pinout");
      return;
    }
    int r = doc["r"] | 0;
    int g = doc["g"] | 0;
    int b = doc["b"] | 0;
    if (rPin >= 0 && gPin >= 0 && bPin >= 0) {
      GpioControl::analogSet(rPin, r);
      GpioControl::analogSet(gPin, g);
      GpioControl::analogSet(bPin, b);
      Serial.printf("[GPIO] RGB R=%d(pin%d) G=%d(pin%d) B=%d(pin%d)\n",
                    r, rPin, g, gPin, b, bPin);
      GpioControl::publishStatus();
    } else {
      Serial.println("[GPIO] RGB: rPin/gPin/bPin all required");
    }
  } else if (cmd == "servo") {
    int pin = doc["pin"] | -1;
    if (pin >= 0 && !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "GPIO", "REJECTED: pin=%d not in external pinout", pin);
      return;
    }
    int angle = doc["angle"] | 90;
    if (pin >= 0) {
      ServoEntry *se = findOrAddServo(pin);
      if (se) {
        if (!se->attached) {
          se->servo.attach(pin);
          se->attached = true;
          Serial.printf("[GPIO] SERVO attached pin=%d\n", pin);
        }
        se->servo.write(angle);
        se->angle = angle;
        Serial.printf("[GPIO] SERVO pin=%d angle=%d\n", pin, angle);
      }
    } else {
      Serial.println("[GPIO] SERVO: pin required");
    }
  } else if (cmd == "servo_detach") {
    int pin = doc["pin"] | -1;
    if (pin >= 0 && !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "GPIO", "REJECTED: servo_detach pin=%d", pin);
      return;
    }
    if (pin >= 0) {
      bool found = false;
      for (int i = 0; i < servoCount; i++) {
        if (servoList[i].pin == pin) {
          if (servoList[i].attached) {
            servoList[i].servo.detach();
            servoList[i].attached = false;
          }
          found = true;
          break;
        }
      }
      // �?不管 servoList 里有没有，都强制释放 LEDC 通道
      ledcDetach(pin);
      pinMode(pin, OUTPUT);
      digitalWrite(pin, LOW);
      Serial.printf("[GPIO] SERVO_DETACH pin=%d (found=%s)\n",
                    pin, found ? "yes" : "no");
    }
  }


  else if (cmd == "pulse_in") {
    int pin = doc["pin"] | -1;
    if (pin < 0 || !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "PULSE", "REJECTED: invalid pin");
      return;
    }
    int trigPin = doc["trig"] | -1;
    if (trigPin >= 0 && !isValidExternalPin(trigPin)) {
      MqttClient::publishLog("warn", "PULSE", "REJECTED: trig=%d", trigPin);
      return;
    }
    String state = doc["state"] | String("high");
    int timeout = doc["timeout"] | 30000;
    int samples = doc["samples"] | 1;
    if (samples < 1) samples = 1;
    if (samples > 20) samples = 20;

    int level = (state == "low") ? LOW : HIGH;
    long medianDuration = GpioControl::measurePulse(pin, trigPin, level, timeout, samples);

    if (doc.containsKey("result_var")) {
      String varName = doc["result_var"].as<String>();
      if (varName.length() > 0)
        ScriptEngine::setVar(varName, (float)medianDuration, false);
    }

    JsonDocument resp;
    resp["type"] = "pulse_in";
    resp["deviceId"] = getDeviceId();
    resp["pin"] = pin;
    resp["state"] = state;
    resp["duration"] = medianDuration;
    resp["samples"] = samples;
    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
      MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
    Serial.printf("[PULSE] pin=%d state=%s duration=%ld us (%d samples)\n",
                  pin, state.c_str(), medianDuration, samples);
  }

  // �?新增：音频输�?
  else if (cmd == "tone") {
    int pin = doc["pin"] | -1;
    if (pin < 0 || !isValidExternalPin(pin)) {
      MqttClient::publishLog("warn", "TONE", "REJECTED: pin=%d", pin);
      return;
    }
    unsigned int freq = doc["freq"] | 0;
    GpioControl::toneSet(pin, freq);
    Serial.printf("[TONE] pin=%d freq=%u\n", pin, freq);
  }




  else if (cmd == "batch") {
    int batchDepth = 0;
    if (doc.containsKey("_bd")) batchDepth = doc["_bd"].as<int>();
    if (batchDepth >= 3) {
      Serial.println("[BATCH] Max depth 3 reached, skipped");
      return;
    }
    JsonArray cmds = doc["commands"].as<JsonArray>();
    Serial.printf("[GPIO] BATCH: %u commands (depth=%d)\n", cmds.size(), batchDepth);
    for (JsonObject c : cmds) {
      JsonDocument tmp;
      tmp.set(c);
      tmp["_bd"] = batchDepth + 1;
      handleCommand(tmp, fromMqtt);
    }
  }


  else if (cmd == "clear") {
    Serial.println("[GPIO] Clearing all pins");

    // 1. 保存定时器原始状态，然后禁用
    std::vector<bool> timerStates;
    for (auto &t : TimerEngine::getList()) {
      timerStates.push_back(t.enabled);
      t.enabled = false;
    }

    // 2. 保存逻辑规则原始状态，然后禁用
    std::vector<bool> logicStates;
    for (auto &r : LogicEngine::getList()) {
      logicStates.push_back(r.enabled);
      r.enabled = false;
    }

    // 3. 清除所有引�?
    for (int pi = 0; pi < validExternalPinsCount; pi++) {
      int pin = validExternalPins[pi];
      int mode = GpioControl::getPinMode(pin);
      if (mode == OUTPUT) {
        analogWrite(pin, 0);
        digitalWrite(pin, LOW);
        Serial.printf("[GPIO] Cleared pin %d\n", pin);
      }
    }

    // 4. 分离所有舵�?
    for (int i = 0; i < servoCount; i++) {
      if (servoList[i].attached) {
        servoList[i].servo.detach();
        servoList[i].attached = false;
        Serial.printf("[GPIO] Detached servo pin %d\n", servoList[i].pin);
      }
    }

    // 5. 重置 GPIO 运行时记�?
    GpioControl::init();

    // 6. 原样恢复定时器状态（不改 RTC 相关的触发时间）
    for (size_t i = 0; i < TimerEngine::getList().size(); i++) {
      auto &t = TimerEngine::getList()[i];
      t.enabled = timerStates[i];
      // 如果原来是启用的，重置执行计数让它从干净状态重新开�?
      if (t.enabled) {
        t.executed = 0;
        t.lastRun = millis();
        t.startTime = millis();
      }
    }

    // 7. 原样恢复逻辑规则状�?
    for (size_t i = 0; i < LogicEngine::getList().size(); i++) {
      auto &r = LogicEngine::getList()[i];
      r.enabled = logicStates[i];
      // 如果原来是启用的，重置边沿检测从干净状态重新判�?
      if (r.enabled) {
        r.prevCondition = false;
        r.lastTrigger = 0;
      }
    }

    // 8. RTC 定时任务不动（它们触发时间没到就不会执行，和引脚状态无关）

    // 9. 发布状�?
    GpioControl::publishStatus();

    Serial.println("[GPIO] All cleared, tasks restored to original state");
  }


  else if (cmd == "dump") {
    JsonDocument doc;
    doc["type"] = "config_dump";
    doc["deviceId"] = getDeviceId();
    doc["deviceName"] = config.deviceName;

    // WiFi
    doc["staSsid"] = config.staSsid;
    doc["apHidden"] = config.apHidden;

    // MQTT
    doc["mqttHost"] = config.mqttHost;
    doc["mqttPort"] = config.mqttPort;
    doc["mqttUser"] = config.mqttUser;
    doc["mqttSsl"] = config.mqttSsl;
    doc["mqttStatus"] = MqttClient::isConnected() ? "connected" : "disconnected";

    // 订阅主题
    JsonArray subs = doc["subTopics"].to<JsonArray>();
    for (auto &t : config.subTopics) {
      JsonObject o = subs.add<JsonObject>();
      o["topic"] = t.topic;
      o["qos"] = t.qos;
    }

    // 发布主题
    JsonArray pubs = doc["pubTopics"].to<JsonArray>();
    for (auto &t : config.pubTopics) {
      JsonObject o = pubs.add<JsonObject>();
      o["topic"] = t.topic;
      o["qos"] = t.qos;
    }

    // 传感�?
    JsonArray snrs = doc["sensors"].to<JsonArray>();
    for (auto &s : config.sensors) {
      JsonObject o = snrs.add<JsonObject>();
      o["pin"] = s.pin;
      o["interval"] = s.interval;
      o["type"] = s.type;
      o["enabled"] = s.enabled;
      o["label"] = s.label;
      o["persistent"] = s.persistent;
    }

    // 输入
    JsonArray ins = doc["inputs"].to<JsonArray>();
    for (auto &t : config.inputs) {
      JsonObject o = ins.add<JsonObject>();
      o["pin"] = t.pin;
      o["mode"] = t.mode;
      o["debounceMs"] = t.debounceMs;
      o["enabled"] = t.enabled;
      o["label"] = t.label;
      o["persistent"] = t.persistent;
    }

    // 定时�?
    JsonArray tmrs = doc["timers"].to<JsonArray>();
    for (auto &te : config.timers) {
      JsonObject o = tmrs.add<JsonObject>();
      o["id"] = te.id;
      o["type"] = te.type;
      o["interval"] = te.interval;
      o["count"] = te.count;
      o["enabled"] = te.enabled;
      o["persistent"] = te.persistent;
      JsonDocument cmds;
      if (!deserializeJson(cmds, te.commandsJson)) {
        o["commands"] = cmds;
      }
    }

    // 逻辑规则
    JsonArray rules = doc["logicRules"].to<JsonArray>();
    for (auto &lr : config.logicRules) {
      JsonObject o = rules.add<JsonObject>();
      o["id"] = lr.id;
      o["operator"] = lr.operator_;
      o["enabled"] = lr.enabled;
      o["cooldown"] = lr.cooldown;
      o["persistent"] = lr.persistent;
      JsonArray conds = o["conditions"].to<JsonArray>();
      for (auto &c : lr.conditions) {
        JsonObject co = conds.add<JsonObject>();
        co["source"] = c.source;
        co["pin"] = c.pin;
        co["op"] = c.op;
        co["value"] = c.value;
      }
      JsonDocument acts;
      if (!deserializeJson(acts, lr.actionsJson)) {
        o["actions"] = acts;
      }
    }

    // I2C
    JsonArray i2cs = doc["i2cConfigs"].to<JsonArray>();
    for (auto &e : config.i2cConfigs) {
      JsonObject o = i2cs.add<JsonObject>();
      o["sda"] = e.sda;
      o["scl"] = e.scl;
      o["enabled"] = e.enabled;
    }

    // 1-Wire
    JsonArray ows = doc["owConfigs"].to<JsonArray>();
    for (auto &e : config.owConfigs) {
      JsonObject o = ows.add<JsonObject>();
      o["pin"] = e.pin;
      o["enabled"] = e.enabled;
    }

    // UART
    JsonArray uarts = doc["uartConfigs"].to<JsonArray>();
    for (auto &e : config.uartConfigs) {
      JsonObject o = uarts.add<JsonObject>();
      o["port"] = e.port;
      o["tx"] = e.txPin;
      o["rx"] = e.rxPin;
      o["baud"] = e.baud;
      o["dataBits"] = e.dataBits;
      o["stopBits"] = e.stopBits;
      o["parity"] = e.parity;
      o["enabled"] = e.enabled;
      o["listening"] = e.listening;
    }

    // SPI
    JsonArray spis = doc["spiConfigs"].to<JsonArray>();
    for (auto &e : config.spiConfigs) {
      JsonObject o = spis.add<JsonObject>();
      o["port"] = e.port;
      o["mosi"] = e.mosiPin;
      o["miso"] = e.misoPin;
      o["sclk"] = e.sclkPin;
      o["speed"] = e.speed;
      o["mode"] = e.mode;
      o["enabled"] = e.enabled;
    }

    // Touch
    JsonArray touches = doc["touchPins"].to<JsonArray>();
    for (auto &e : config.touchPins) {
      JsonObject o = touches.add<JsonObject>();
      o["pin"] = e.pin;
      o["threshold"] = e.threshold;
      o["debounceMs"] = e.debounceMs;
      o["enabled"] = e.enabled;
      o["label"] = e.label;
      o["persistent"] = e.persistent;
    }


    // 系统信息
    doc["batchInterval"] = config.batchInterval;
    doc["heapFree"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;
    doc["firmware"] = FIRMWARE_VERSION;

    String payload;
    serializeJson(doc, payload);
    publishResult(payload);
    Serial.printf("[DUMP] Published full config (%d bytes)\n", payload.length());


  }


  else if (cmd == "status") {
    GpioControl::publishStatus();
  } else if (cmd == "restart") {
    Serial.println("[GPIO] Restarting...");
    delay(500);
    ESP.restart();
  } else if (cmd == "custom") {
    String action = doc["action"].as<String>();
    Serial.printf("[GPIO] Custom action: %s\n", action.c_str());
    if (action == "reset") {
      config.reset();
      Serial.println("[GPIO] Config reset, restarting...");
      delay(500);
      ESP.restart();
    } else if (action == "query_mqtt_ota_topic") {
      JsonDocument resp;
      resp["type"] = "mqtt_ota_topic";
      resp["deviceId"] = getDeviceId();
      resp["otaTopic"] = MQTT_OTA_TOPIC;
      resp["otaReportTopic"] = MQTT_OTA_REPORT_TOPIC;
      String p;
      serializeJson(resp, p);
      publishResult(p);

      Serial.printf("[MQTT-OTA] Reserved topic: %s, Report: %s\n",

                    MQTT_OTA_TOPIC, MQTT_OTA_REPORT_TOPIC);
    } else if (action == "query_pin_capabilities") {
      JsonDocument resp;
      resp["type"] = "pin_capabilities";
      resp["deviceId"] = getDeviceId();
      resp["supportsPwm"] = true;
      resp["supportsServo"] = true;
      resp["supportsAdc"] = true;
      resp["supportsTouch"] = true;
      resp["maxPwmValue"] = 255;
      resp["maxServoAngle"] = 180;
      resp["adcResolution"] = 4095;
      String p;
      serializeJson(resp, p);
      if (mqttClient.connected() && config.pubTopics.size() > 0) {
        publishResult(p);
      }
      Serial.println("[GPIO] Pin capabilities published");

    } else if (action == "query_batch_config") {
      JsonDocument resp;
      resp["type"] = "batch_config";
      resp["deviceId"] = getDeviceId();
      resp["batchInterval"] = config.batchInterval;
      String p;
      serializeJson(resp, p);
      if (mqttClient.connected() && config.pubTopics.size() > 0) {
        publishResult(p);
      }
      Serial.printf("[BATCH] Config: interval=%d\n", config.batchInterval);

    } else if (action == "set_batch_interval") {
      int interval = doc["interval"] | 0;
      config.batchInterval = interval;
      config.markDirty();
      Serial.printf("[BATCH] Interval set to %d s\n", interval);
    }
  } 
  else if (cmd == "log") {
    String message = doc["message"].as<String>();
    String level = doc["level"] | String("info");
    Serial.printf("[LOG][%s] %s\n", level.c_str(), message.c_str());
    JsonDocument resp;
    resp["type"] = "log";
    resp["level"] = level;
    resp["message"] = message;
    resp["deviceId"] = getDeviceId();
    String p;
    serializeJson(resp, p);
    publishResult(p);
  }
  else if (cmd == "sensor") {
    handleSensorCommand(doc);
  } else if (cmd == "input") {
    handleInputCommand(doc);
  } else if (cmd == "timer") {
    handleTimerCommand(doc);
  } else if (cmd == "logic") {
    handleLogicCommand(doc);
  } else if (cmd == "batch_status") {
    publishBatchStatus();
  } else if (cmd == "i2c") {
    I2C::handleCommand(doc);
  } else if (cmd == "onewire") {
    OneWire::handleCommand(doc);
  } else if (cmd == "uart") {
    UART::handleCommand(doc);
  } else if (cmd == "spi") {
    SPIBus::handleCommand(doc);
  } else if (cmd == "touch") {
    Touch::handleCommand(doc);
  } else if (cmd == "encoder") {  // �?新增
    Encoder::handleCommand(doc);
  }
// ========== 新增：display 命令分发 ==========
  else if (cmd == "display") {
    DisplayEngine::handleCommand(doc);
  }

  else if (cmd == "rtc") {
    RTC::handleCommand(doc);
  } else if (cmd == "rand") {
    Rand::handleCommand(doc);
  } else if (cmd == "script") {
    ScriptEngine::handleCommand(doc);
  } else if (cmd == "data") {
    DataEngine::handleCommand(doc);
  } 
  
 else if (cmd == "module") {
    String action = doc["action"].as<String>();
    String moduleType = doc["module_type"].as<String>();
    if (moduleType.length() == 0) moduleType = doc["moduleType"].as<String>();

    int pin = doc["pin"] | -1;
    String label = doc["label"].as<String>();
    int interval = doc["interval"] | 5000;

    if (action == "add") {
      if (pin < 0) {
        Serial.println("[MODULE] pin required");
        return;
      }
      if (findModule(pin)) {
        Serial.printf("[MODULE] pin=%d already registered\n", pin);
        return;
      }

      ModuleEntry entry;
      entry.pin = pin;
      entry.moduleType = moduleType;
      entry.label = label.length() > 0 ? label : moduleType;
      entry.interval = interval;
      entry.enabled = true;

      if (moduleType == "ds18b20") {
        JsonDocument owCmd;
        owCmd["cmd"] = "onewire";
        owCmd["action"] = "search";
        owCmd["pin"] = pin;
        OneWire::handleCommand(owCmd);
        moduleRegistry.push_back(entry);
        Serial.printf("[MODULE] DS18B20 added on pin=%d\n", pin);

      } else if (moduleType == "dht11" || moduleType == "dht22") {
        moduleRegistry.push_back(entry);
        Serial.printf("[MODULE] %s added on pin=%d interval=%d label=%s\n",
                      moduleType.c_str(), pin, interval, entry.label.c_str());

      } else {
        Serial.printf("[MODULE] Unknown module type: %s\n", moduleType.c_str());
        return;
      }

      JsonDocument resp;
      resp["type"] = "module_add";
      resp["deviceId"] = getDeviceId();
      resp["moduleType"] = moduleType;
      resp["pin"] = pin;
      resp["label"] = entry.label;
      resp["interval"] = entry.interval;
      resp["enabled"] = true;
      String p;
      serializeJson(resp, p);
      publishResult(p);

    } else if (action == "remove") {
      if (pin < 0) {
        Serial.println("[MODULE] pin required");
        return;
      }
      bool found = false;
      for (auto it = moduleRegistry.begin(); it != moduleRegistry.end(); ++it) {
        if (it->pin == pin) {
          if (it->moduleType == "ds18b20") {
            JsonDocument owCmd;
            owCmd["cmd"] = "onewire";
            owCmd["action"] = "reset";
            owCmd["pin"] = pin;
            OneWire::handleCommand(owCmd);
          }
          Serial.printf("[MODULE] Removed %s on pin=%d\n", it->moduleType.c_str(), pin);
          moduleRegistry.erase(it);
          found = true;
          break;
        }
      }
      if (!found) {
        Serial.printf("[MODULE] No module on pin=%d\n", pin);
        return;
      }

      JsonDocument resp;
      resp["type"] = "module_remove";
      resp["deviceId"] = getDeviceId();
      resp["pin"] = pin;
      String p;
      serializeJson(resp, p);
      publishResult(p);

    } else if (action == "enable") {
      if (pin < 0) {
        Serial.println("[MODULE] pin required");
        return;
      }
      bool en = doc["enabled"] | true;
      ModuleEntry *m = findModule(pin);
      if (!m) {
        Serial.printf("[MODULE] No module on pin=%d\n", pin);
        return;
      }
      m->enabled = en;
      Serial.printf("[MODULE] %s on pin=%d %s\n",
                    m->moduleType.c_str(), pin, en ? "enabled" : "disabled");

      JsonDocument resp;
      resp["type"] = "module_enable";
      resp["deviceId"] = getDeviceId();
      resp["pin"] = pin;
      resp["enabled"] = en;
      String p;
      serializeJson(resp, p);
      publishResult(p);

    } else if (action == "read") {
      if (pin < 0) {
        Serial.println("[MODULE] pin required");
        return;
      }

      ModuleEntry *m = findModule(pin);
      if (m && !m->enabled) {
        Serial.printf("[MODULE] Module on pin=%d is disabled\n", pin);
        return;
      }

      if (moduleType == "ds18b20" || (m && m->moduleType == "ds18b20")) {
        JsonDocument owCmd;
        owCmd["cmd"] = "onewire";
        owCmd["action"] = "read_temp";
        owCmd["pin"] = pin;
        if (doc.containsKey("rom")) {
          owCmd["rom"] = doc["rom"].as<String>();
        }
        OneWire::handleCommand(owCmd);

      } else if (moduleType == "dht11" || moduleType == "dht22"
                 || (m && (m->moduleType == "dht11" || m->moduleType == "dht22"))) {
        String typeToUse = moduleType.length() > 0 ? moduleType : (m ? m->moduleType : "dht11");

        float humidity = -1.0f;
        float temperature = -1.0f;

        pinMode(pin, OUTPUT);
        unsigned long startTime = millis();

        digitalWrite(pin, LOW);
        delay(18);
        digitalWrite(pin, HIGH);
        delayMicroseconds(40);
        pinMode(pin, INPUT_PULLUP);

        while (digitalRead(pin) == HIGH && millis() - startTime < 100) { delayMicroseconds(1); yield(); }
        if (millis() - startTime >= 100) {
          Serial.println("[MODULE] DHT timeout");
          return;
        }

        while (digitalRead(pin) == LOW && millis() - startTime < 100) { delayMicroseconds(1); yield(); }
        while (digitalRead(pin) == HIGH && millis() - startTime < 100) { delayMicroseconds(1); yield(); }

        uint8_t data[5] = {0};
        for (int i = 0; i < 40; i++) {
          while (digitalRead(pin) == LOW && millis() - startTime < 100) { delayMicroseconds(1); yield(); }
          unsigned long t = micros();
          while (digitalRead(pin) == HIGH && millis() - startTime < 100) { delayMicroseconds(1); yield(); }
          unsigned long duration = micros() - t;
          data[i / 8] = (data[i / 8] << 1) | (duration > 40 ? 1 : 0);
        }

        if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3])) {
          Serial.println("[MODULE] DHT checksum error");
          return;
        }

        if (typeToUse == "dht11") {
          humidity = data[0];
          temperature = data[2];
        } else {
          humidity = data[0] + data[1] * 0.1f;
          int16_t tempRaw = (data[2] & 0x7F) << 8 | data[3];
          if (data[2] & 0x80) tempRaw = -tempRaw;
          temperature = tempRaw / 10.0f;
        }

        Serial.printf("[MODULE] DHT%s pin=%d Temp=%.1f Humidity=%.1f\n",
                      typeToUse.c_str(), pin, temperature, humidity);

        JsonDocument resp;
        resp["type"] = "module_data";
        resp["deviceId"] = getDeviceId();
        resp["moduleType"] = typeToUse;
        resp["pin"] = pin;
        resp["temperature"] = temperature;
        resp["humidity"] = humidity;
        if (doc.containsKey("result_var")) {
          String varName = doc["result_var"].as<String>();
          if (varName.length() > 0) {
            ScriptEngine::setVar(varName, temperature, false);
          }
        }
        if (doc.containsKey("result_var_hum")) {
          String varName = doc["result_var_hum"].as<String>();
          if (varName.length() > 0) {
            ScriptEngine::setVar(varName, humidity, false);
          }
        }
        String p;
        serializeJson(resp, p);
        publishResult(p);

      } else {
        Serial.printf("[MODULE] Unknown module type for read: %s\n", moduleType.c_str());
      }

    } else if (action == "list") {
      JsonDocument resp;
      resp["type"] = "module_list";
      resp["deviceId"] = getDeviceId();
      JsonArray arr = resp["modules"].to<JsonArray>();
      for (auto &m : moduleRegistry) {
        JsonObject o = arr.add<JsonObject>();
        o["pin"] = m.pin;
        o["moduleType"] = m.moduleType;
        o["label"] = m.label;
        o["interval"] = m.interval;
        o["enabled"] = m.enabled;
      }
      String p;
      serializeJson(resp, p);
      publishResult(p);
      Serial.printf("[MODULE] List published (%d modules)\n", moduleRegistry.size());

    } else {
      Serial.printf("[MODULE] Unknown action: %s\n", action.c_str());
    }
  }else {
    MqttClient::publishLog("warn", "GPIO", "Unknown cmd: %s", cmd.c_str());
  }
  // ===== ACK 响应 =====
  if (doc.containsKey("_mid")) {
    String midStr = doc["_mid"].as<String>();
    if (midStr.length() > 0) {
      JsonDocument ack;
      ack["type"] = "ack";
      ack["_mid"] = midStr;
      ack["status"] = "ok";
      ack["deviceId"] = getDeviceId();
      String ackJson;
      serializeJson(ack, ackJson);
      if (mqttClient.connected() && config.pubTopics.size() > 0) {
        publishResult(ackJson);
      }
    }
  }
}

// 串口输入处理
static void checkSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        Serial.printf("\n[SERIAL] Received: %s\n", serialBuffer.c_str());
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, serialBuffer);
        if (err) {
          Serial.printf("[SERIAL] JSON error: %s\n", err.c_str());
        } else {
          String cmd = doc["cmd"].as<String>();
          if (cmd == "sensor") {
            handleSensorCommand(doc);
          } else if (cmd == "input") {
            handleInputCommand(doc);
          } else if (cmd == "timer") {
            handleTimerCommand(doc);
          } else if (cmd == "logic") {
            handleLogicCommand(doc);
          } else {
            handleCommand(doc);
          }
        }
        serialBuffer = "";
      }
    } else {
      if (serialBuffer.length() < 2048) serialBuffer += c;
    }
  }
}

// MQTT OTA 处理
static void otaReset() {
  mqttOtaActive = false;
  otaInProgress = false;
  otaTotalSize = 0;
  otaChunks = 0;
  otaChunkCount = 0;
  otaWritten = 0;
}

static void otaReport(const String &status, int progress, uint32_t chunk = 0) {
  JsonDocument doc;
  doc["status"] = status;
  doc["progress"] = progress;
  doc["chunk"] = chunk;
  doc["written"] = otaWritten;
  doc["total"] = otaTotalSize;
  String json;
  serializeJson(doc, json);
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_OTA_REPORT_TOPIC, json.c_str());
  }
  Serial.printf("[MQTT-OTA] Report: %s %d%% (written=%u)\n",
                status.c_str(), progress, otaWritten);
}

static void handleOtaMessage(const uint8_t *payload, unsigned int length) {
  if (payload == nullptr || length == 0) return;

  Serial.printf("[MQTT-OTA] Received %u bytes\n", length);

  if (length > 5 && payload[0] == 0x01) {
    if (!mqttOtaActive) {
      Serial.println("[MQTT-OTA] Data without START - ignoring");
      return;
    }

    uint32_t chunkIndex = 0;
    memcpy(&chunkIndex, &payload[1], 4);

    const uint8_t *data = &payload[5];
    uint32_t dataLen = length - 5;

    otaLastChunk = millis();

    if (!Update.isRunning()) {
      Serial.println("[MQTT-OTA] Update not running!");
      otaReset();
      otaReport("error", 0, chunkIndex);
      return;
    }

    size_t written = Update.write(const_cast<uint8_t *>(data), dataLen);
    if (written != dataLen) {
      Serial.printf("[MQTT-OTA] Write FAIL chunk %u (wrote %u/%u)\n",
                    chunkIndex, written, dataLen);
      Update.printError(Serial);
      Update.abort();
      otaReset();
      otaReport("error", 0, chunkIndex);
      return;
    }

    otaWritten += dataLen;
    otaChunkCount = chunkIndex + 1;

    int progress = 0;
    if (otaTotalSize > 0) {
      progress = (int)((otaWritten * 100) / otaTotalSize);
      if (progress > 99) progress = 99;
    }

    if (chunkIndex % 64 == 0) {
      Serial.printf("[MQTT-OTA] Chunk %u, %u/%u bytes, %d%%\n",
                    chunkIndex, otaWritten, otaTotalSize, progress);
    }

    otaReport("receiving", progress, chunkIndex);
    return;
  }

  String cmd = "";
  cmd.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    cmd += (char)payload[i];
  }
  Serial.printf("[MQTT-OTA] Text cmd: %s\n", cmd.c_str());

  if (cmd.indexOf("START") >= 0) {
    Serial.println("[MQTT-OTA] START command received");

    if (otaInProgress || mqttOtaActive) {
      Serial.println("[MQTT-OTA] Cleaning stale OTA state");
      Update.abort();
      otaReset();
    }

    otaTotalSize = 0;
    otaChunks = 0;

    if (cmd.startsWith("{")) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, cmd);
      if (err) {
        Serial.printf("[MQTT-OTA] JSON parse error: %s\n", err.c_str());
        otaReport("error", 0);
        return;
      }

      otaTotalSize = doc["size"] | 0;
      otaChunks = doc["chunks"] | 0;

      const char *targetPtr = doc["target"];
      if (targetPtr && strlen(targetPtr) > 0) {
        String myId = getDeviceId();
        if (String(targetPtr) != myId) {
          Serial.printf("[MQTT-OTA] Not for me (target=%s, I am %s)\n",
                        targetPtr, myId.c_str());
          return;
        }
      }
    }

    Serial.printf("[MQTT-OTA] Size=%u, Chunks=%u, FreeHeap=%u\n",
                  otaTotalSize, otaChunks, ESP.getFreeHeap());

    bool beginOk = false;
    if (otaTotalSize > 0) {
      beginOk = Update.begin(otaTotalSize);
    } else {
      beginOk = Update.begin(UPDATE_SIZE_UNKNOWN);
    }

    if (beginOk) {
      otaInProgress = true;
      mqttOtaActive = true;
      otaWritten = 0;
      otaChunkCount = 0;
      otaLastChunk = millis();
      Serial.println("[MQTT-OTA] Update.begin OK");
      otaReport("started", 0);
    } else {
      Serial.println("[MQTT-OTA] Update.begin FAILED!");
      Update.printError(Serial);
    }
    return;
  }

  if (cmd == "DONE") {
    if (!mqttOtaActive) {
      Serial.println("[MQTT-OTA] DONE without START");
      return;
    }

    Serial.printf("[MQTT-OTA] Done, %u bytes, %u chunks\n",
                  otaWritten, otaChunkCount);
    otaReport("finalizing", 99);

    if (Update.end(true)) {
      Serial.println("[MQTT-OTA] SUCCESS - restarting");
      otaReport("success", 100);
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("[MQTT-OTA] Update.end FAILED");
      Update.printError(Serial);
      otaReset();
      otaReport("error", 0);
    }
    return;
  }

  if (cmd == "ABORT") {
    Serial.println("[MQTT-OTA] Aborted");
    Update.abort();
    otaReset();
    otaReport("aborted", 0);
    return;
  }

  Serial.printf("[MQTT-OTA] Unknown command: %s\n", cmd.c_str());
}

// MQTT 消息回调
static void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("[MQTT] Data: %s = %s\n", topic, msg.c_str());

  if (!msg.startsWith("{")) return;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.printf("[MQTT] JSON error: %s\n", err.c_str());
    return;
  }

  if (doc.containsKey("_from")) {
    String from = doc["_from"].as<String>();
    if (from.length() > 0 && from == getDeviceId()) {
      Serial.printf("[MQTT] Echo filtered (from self) on %s\n", topic);
      return;
    }
    if (from.length() > 0) {
      DualChannel::updateMqttStatus(from, true);
    }
  }

  String cmd = doc["cmd"].as<String>();
  if (cmd == "sensor") {
    handleSensorCommand(doc);
  } else if (cmd == "input") {
    handleInputCommand(doc);
  } else if (cmd == "timer") {
    handleTimerCommand(doc);
  } else if (cmd == "logic") {
    handleLogicCommand(doc);
  } else {
    handleCommand(doc, true);
  }
}

static void handleLwtMessage(byte *payload, unsigned int length) {
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  JsonDocument doc;
  if (deserializeJson(doc, msg)) return;

  String fromId = doc["deviceId"].as<String>();
  if (fromId.length() == 0 || fromId == getDeviceId()) return;

  bool online = doc["online"].as<bool>();
  String name = doc.containsKey("deviceName") ? doc["deviceName"].as<String>() : "";
  DualChannel::updateMqttStatus(fromId, online, name);
  Serial.printf("[MQTT] LWT: %s %s\n", fromId.c_str(), online ? "ONLINE" : "OFFLINE");
}

static void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.printf("[MQTT] Msg: %s (%u bytes)\n", topic, length);

  if (strcmp(topic, MQTT_OTA_TOPIC) == 0) {
    handleOtaMessage((const uint8_t *)payload, length);
    return;
  }

  if (strcmp(topic, LWT_TOPIC) == 0) {
    handleLwtMessage(payload, length);
    return;
  }

  onMqttMessage(topic, payload, length);
}

// MqttClient 命名空间
namespace MqttClient {

void init() {
  connected = false;
  mqttOtaActive = false;
  otaInProgress = false;
  otaWritten = 0;
  otaTotalSize = 0;
  lastBatchReport = millis();
  mqttState = MQTT_IDLE;
  mqttStateTime = millis();
  mqttReconnectBackoff = 2000;
}

void loop() {

  // MQTT 开关：关闭时不处理任何 MQTT 逻辑
  if (!config.mqttEnabled) {
    if (connected) {
      mqttClient.disconnect();
      connected = false;
      mqttState = MQTT_IDLE;
      Serial.println("[MQTT] Disabled by config");
    }
    return;
  }
  // MQTT 未连接时，允许串口命�?
  if (!mqttClient.connected()) {
    checkSerialInput();
  }

  if (!WiFi.isConnected()) {
    if (connected) {
      Serial.println("[MQTT] No WiFi");
      connected = false;
      mqttState = MQTT_IDLE;
      mqttStateTime = millis();
      mqttReconnectBackoff = 2000;
      sslPreConnected = false;
    }
    return;
  }

  // 已连接：正常处理
  if (mqttState == MQTT_LINKED && mqttClient.connected()) {
    mqttClient.loop();

    // MQTT OTA 超时检�?
    if (mqttOtaActive && millis() - otaLastChunk > 30000) {
      Serial.println("[MQTT-OTA] Timeout - aborting");
      Update.abort();
      otaReset();
      otaReport("timeout", 0);
    }

    // 批量状态上�?
    if (config.batchInterval > 0) {
      unsigned long intervalMs = (unsigned long)config.batchInterval * 1000;
      if (millis() - lastBatchReport >= intervalMs) {
        lastBatchReport = millis();
        publishBatchStatus();
      }
    }

    // 检测连接断开
    if (!mqttClient.connected()) {
      Serial.println("[MQTT] Connection lost");
      connected = false;
      mqttState = MQTT_IDLE;
      mqttStateTime = millis();
      mqttReconnectBackoff = 2000;
      sslPreConnected = false;
    }
    return;
  }

  // 未连接：非阻塞重连状态机
  connected = false;
  unsigned long now = millis();

  switch (mqttState) {

    case MQTT_IDLE:
      {
        if (now - mqttStateTime < mqttReconnectBackoff) return;

        if (config.mqttHost.length() == 0) {
          mqttStateTime = now;
          mqttReconnectBackoff = MQTT_MAX_BACKOFF;
          Serial.println("[MQTT] No host configured, waiting...");
          return;
        }

        pendingClientId = config.mqttClientId.length() > 0
                            ? config.mqttClientId
                            : getDeviceId();

        if (config.mqttSsl) {
          sslClient.setInsecure();
          mqttClient.setClient(sslClient);
        } else {
          mqttClient.setClient(wifiClient);
        }
        mqttClient.setCallback(mqttCallback);
        mqttClient.setBufferSize(4096);
        sslPreConnected = false;


        Serial.printf("[MQTT] Resolving %s ...\n", config.mqttHost.c_str());
        mqttState = MQTT_RESOLVING;
        mqttStateTime = now;
        break;
      }

    case MQTT_RESOLVING:
      {
        if (now - mqttStateTime > 5000) {
          Serial.println("[MQTT] DNS timeout");
          mqttState = MQTT_IDLE;
          mqttStateTime = now;
          mqttReconnectBackoff = min(mqttReconnectBackoff * 2, MQTT_MAX_BACKOFF);
          Serial.printf("[MQTT] Backoff: %lu ms\n", mqttReconnectBackoff);
          return;
        }

        int ret = WiFi.hostByName(config.mqttHost.c_str(), resolvedIP);

        if (ret == 0 || (resolvedIP[0] == 0 && resolvedIP[1] == 0 && resolvedIP[2] == 0 && resolvedIP[3] == 0)) {
          Serial.println("[MQTT] DNS failed");
          mqttState = MQTT_IDLE;
          mqttStateTime = now;
          mqttReconnectBackoff = min(mqttReconnectBackoff * 2, MQTT_MAX_BACKOFF);
          Serial.printf("[MQTT] Backoff: %lu ms\n", mqttReconnectBackoff);
          return;
        }

        Serial.printf("[MQTT] Resolved to %s\n", resolvedIP.toString().c_str());

        mqttClient.setServer(resolvedIP, config.mqttPort);
        mqttState = MQTT_CONNECTING;
        mqttStateTime = now;
        break;
      }

    case MQTT_CONNECTING:
      {
        if (now - mqttStateTime > 10000) {
          Serial.println("[MQTT] Connect timeout");
          int rc = mqttClient.state();
          Serial.printf("[MQTT] rc=%d (%s)\n", rc, mqttErrorString(rc));
          mqttClient.disconnect();
          sslPreConnected = false;
          mqttState = MQTT_IDLE;
          mqttStateTime = now;
          mqttReconnectBackoff = min(mqttReconnectBackoff * 2, MQTT_MAX_BACKOFF);
          Serial.printf("[MQTT] Backoff: %lu ms\n", mqttReconnectBackoff);
          return;
        }

        // SSL: 先用域名建立连接（带 SNI），再让 PubSubClient �?MQTT
        if (config.mqttSsl && !sslPreConnected) {
          Serial.printf("[MQTT-SSL] Connecting to %s:%u ...\n",
                        config.mqttHost.c_str(), config.mqttPort);
          sslClient.stop();
          delay(50);
          sslClient.setInsecure();
          if (!sslClient.connect(config.mqttHost.c_str(), config.mqttPort, 10000)) {
            Serial.println("[MQTT-SSL] SSL connect failed");
            mqttClient.disconnect();
            sslPreConnected = false;
            mqttState = MQTT_IDLE;
            mqttStateTime = now;
            mqttReconnectBackoff = min(mqttReconnectBackoff * 2, MQTT_MAX_BACKOFF);
            Serial.printf("[MQTT] Backoff: %lu ms\n", mqttReconnectBackoff);
            return;
          }
          Serial.println("[MQTT-SSL] SSL connected OK");
          sslPreConnected = true;
        }

        // PubSubClient 检测到 socket 已连接，直接�?MQTT CONNECT
        Serial.printf("[MQTT] %s %s:%u (ClientID=%s)\n",
                      config.mqttSsl ? "SSL" : "TCP",
                      config.mqttHost.c_str(),
                      config.mqttPort,
                      pendingClientId.c_str());
        Serial.printf("[MQTT] Free heap: %u bytes\n", ESP.getFreeHeap());

        lwtOnlineMsg = "{\"deviceId\":\"" + getDeviceId() + "\",\"deviceName\":\"" + config.deviceName + "\",\"online\":true}";
        lwtOfflineMsg = "{\"deviceId\":\"" + getDeviceId() + "\",\"online\":false}";

        bool ok = false;
        if (config.mqttUser.length() > 0) {
          Serial.printf("[MQTT] Auth: user=%s\n", config.mqttUser.c_str());
          ok = mqttClient.connect(
            pendingClientId.c_str(),
            config.mqttUser.c_str(),
            config.mqttPass.c_str(),
            LWT_TOPIC,
            1,
            true,
            lwtOfflineMsg.c_str()
          );

        } else {
          Serial.println("[MQTT] Auth: none");
          ok = mqttClient.connect(
            pendingClientId.c_str(),
            NULL, NULL,
            LWT_TOPIC,
            1,
            true,
            lwtOfflineMsg.c_str()
          );
        }

        if (ok) {
          publishLog("info", "MQTT", "Connected!");
          connected = true;
          mqttState = MQTT_LINKED;
          mqttReconnectBackoff = 2000;

          for (auto &t : config.subTopics) {
            mqttClient.subscribe(t.topic.c_str(), t.qos);
            Serial.printf("[MQTT] Sub: %s QoS%d\n", t.topic.c_str(), t.qos);
          }
          mqttClient.subscribe(MQTT_OTA_TOPIC, 1);
          Serial.printf("[MQTT] Sub: %s QoS1 (Reserved OTA)\n", MQTT_OTA_TOPIC);

          mqttClient.subscribe(LWT_TOPIC, 1);
          Serial.printf("[MQTT] Sub: %s QoS1 (LWT)\n", LWT_TOPIC);

          mqttClient.publish(LWT_TOPIC, lwtOnlineMsg.c_str(), true);
          Serial.printf("[MQTT] LWT online published\n");

          lastBatchReport = millis();
        } else {
          int rc = mqttClient.state();
          Serial.printf("[MQTT] Connect failed rc=%d (%s)\n", rc, mqttErrorString(rc));
          Serial.printf("[MQTT] Free heap: %u bytes\n", ESP.getFreeHeap());
          Serial.printf("[MQTT] WiFi RSSI: %d dBm\n", WiFi.RSSI());

          mqttClient.disconnect();
          sslPreConnected = false;
          mqttState = MQTT_IDLE;
          mqttStateTime = now;
          mqttReconnectBackoff = min(mqttReconnectBackoff * 2, MQTT_MAX_BACKOFF);
          Serial.printf("[MQTT] Backoff: %lu ms\n", mqttReconnectBackoff);
        }
        break;
      }


    default:
      mqttState = MQTT_IDLE;
      mqttStateTime = now;
      break;
  }
}


bool isConnected() {
  return mqttClient.connected();
}

void disconnect() {
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  connected = false;
  mqttState = MQTT_IDLE;
  mqttStateTime = millis();
  mqttReconnectBackoff = 2000;
  publishLog("warn", "MQTT", "Disconnected");
}

void publish(const char *topic, const char *payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, payload);
  }
  if (_wsClient != 0xFF && _wsSend && config.pubTopics.size() > 0
      && String(topic) == config.pubTopics[0].topic) {
    _wsSend(_wsClient, payload);
  }
}

void publish(const char *topic, const uint8_t *payload, unsigned int length) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, payload, length);
  }
}

bool publish(const String &topic, const String &payload, uint8_t qos) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic.c_str(), payload.c_str());
  }
  if (_wsClient != 0xFF && _wsSend && config.pubTopics.size() > 0
      && topic == config.pubTopics[0].topic) {
    _wsSend(_wsClient, payload.c_str());
  }
  return mqttClient.connected();
}

void publishLog(const char *level, const char *tag, const char *fmt, ...) {
  char buf[192];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  Serial.printf("[%s] %s\n", tag, buf);

  if (!mqttClient.connected() && _wsClient == 0xFF) return;

  JsonDocument doc;
  doc["type"] = "log";
  doc["deviceId"] = getDeviceId();
  doc["level"] = level;
  doc["tag"] = tag;
  doc["msg"] = buf;
  doc["ts"] = millis();
  String json;
  serializeJson(doc, json);

  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_LOG_TOPIC, json.c_str());
  }
  if (_wsClient != 0xFF && _wsSend) {
    _wsSend(_wsClient, json.c_str());
  }
}


}  // namespace MqttClient
