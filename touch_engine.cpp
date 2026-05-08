#include "touch_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include "pin_caps.h"
#include "script_engine.h"


// ========== 运行时触摸状�?==========
struct TouchState {
  int pin;
  int threshold;
  int debounceMs;
  bool enabled;
  String label;
  bool touched;      // 当前是否被触�?
  bool prevTouched;  // 上一次状�?
  unsigned long lastChange;
  int rawValue;
};

#define MAX_TOUCH_STATES 10
static TouchState touchStates[MAX_TOUCH_STATES];
static int tsCount = 0;

static unsigned long lastCheck = 0;
static const unsigned long CHECK_INTERVAL = 50;  // �?50ms 检查一�?

// ========== 查找运行时状�?==========
static TouchState *findTouchState(int pin) {
  for (int i = 0; i < tsCount; i++) {
    if (touchStates[i].pin == pin) return &touchStates[i];
  }
  return nullptr;
}

// ========== 添加触摸引脚到监�?==========
static TouchState *addTouchState(int pin, int threshold, int debounceMs,
                                 bool enabled, const String &label) {
  // 先检查是否已存在
  TouchState *ts = findTouchState(pin);
  if (ts) {
    ts->threshold = threshold;
    ts->debounceMs = debounceMs;
    ts->enabled = enabled;
    ts->label = label;
    return ts;
  }
  if (tsCount >= MAX_TOUCH_STATES) {
    Serial.println("[TOUCH] Max touch states reached");
    return nullptr;
  }
  TouchState &s = touchStates[tsCount];
  s.pin = pin;
  s.threshold = threshold;
  s.debounceMs = debounceMs;
  s.enabled = enabled;
  s.label = label;
  s.touched = false;
  s.prevTouched = false;
  s.lastChange = millis();
  s.rawValue = 0;
  tsCount++;
  return &s;
}

// ========== 发布触摸事件 ==========
static void publishTouchEvent(const TouchState &ts, const char *event) {
  JsonDocument doc;
  doc["type"] = "touch_event";
  doc["deviceId"] = getDeviceId();
  doc["pin"] = ts.pin;
  doc["event"] = event;  // "touch_down" �?"touch_up"
  doc["raw"] = ts.rawValue;
  doc["threshold"] = ts.threshold;
  doc["label"] = ts.label;
  doc["touched"] = ts.touched;
  String p;
  serializeJson(doc, p);
  if (MqttClient::isConnected() && config.pubTopics.size() > 0)
    MqttClient::publish(config.pubTopics[0].topic, p);
}

namespace Touch {

// ========== 初始化：恢复持久化配�?==========

void init() {
  tsCount = 0;
  lastCheck = 0;

  // �?NVS 恢复持久化触摸引�?
  for (auto &te : config.touchPins) {
    if (!te.enabled || !te.persistent) continue;
    // 验证引脚支持触摸
    const PinCapability *cap = findPinCap(te.pin);
    if (!cap || !cap->touch) {
      Serial.printf("[TOUCH] Skip pin %d (no touch support)\n", te.pin);
      continue;
    }
    addTouchState(te.pin, te.threshold, te.debounceMs, true, te.label);
    Serial.printf("[TOUCH] Restored pin=%d threshold=%d label=%s\n",
                  te.pin, te.threshold, te.label.c_str());
  }
  Serial.printf("[TOUCH] Engine initialized (%d pins)\n", tsCount);
}

// ========== 主循环：触摸检�?==========

void loop() {
  unsigned long now = millis();
  if (now - lastCheck < CHECK_INTERVAL) return;
  lastCheck = now;

  for (int i = 0; i < tsCount; i++) {
    TouchState &ts = touchStates[i];
    if (!ts.enabled) continue;

    int raw = touchRead(ts.pin);
    ts.rawValue = raw;

    // ESP32-S3: 触摸时值降低，touched = raw < threshold
    bool touched = (raw < ts.threshold);

    if (touched != ts.prevTouched) {
      if (now - ts.lastChange >= (unsigned long)ts.debounceMs) {
        ts.lastChange = now;
        ts.touched = touched;
        ts.prevTouched = touched;

        publishTouchEvent(ts, touched ? "touch_down" : "touch_up");
        Serial.printf("[TOUCH] Pin %d %s (raw=%d thr=%d)\n",
                      ts.pin, touched ? "DOWN" : "UP", raw, ts.threshold);
      }
    }
  }
}

// ========== 命令处理 ==========

void handleCommand(JsonDocument &doc) {
  String action = doc["action"].as<String>();

  // ---- add: 添加触摸引脚监控 ----
  if (action == "add") {
    int pin = doc["pin"] | -1;
    int threshold = doc["threshold"] | 0;
    int debounceMs = doc["debounceMs"] | 50;
    String label = doc["label"] | String("");
    bool persist = doc["persistent"] | false;

    if (pin < 0) {
      Serial.println("[TOUCH] add: pin required");
      return;
    }

    // 验证引脚支持触摸
    const PinCapability *cap = findPinCap(pin);
    if (!cap || !cap->touch) {
      Serial.printf("[TOUCH] REJECTED: pin %d does not support touch\n", pin);
      return;
    }

    // 如果未提供阈值，自动校准
    if (threshold <= 0) {
      long sum = 0;
      for (int i = 0; i < 50; i++) {
        sum += touchRead(pin);
        delay(5);
      }
      int avg = sum / 50;
      threshold = avg * 70 / 100;  // 70% 作为阈�?
      Serial.printf("[TOUCH] Auto-calibrated pin %d: avg=%d threshold=%d\n",
                    pin, avg, threshold);
    }

    addTouchState(pin, threshold, debounceMs, true, label);
    Serial.printf("[TOUCH] Added pin=%d threshold=%d debounce=%d label=%s\n",
                  pin, threshold, debounceMs, label.c_str());

    // 持久�?
    if (persist) {
      bool found = false;
      for (auto &te : config.touchPins) {
        if (te.pin == pin) {
          te.threshold = threshold;
          te.debounceMs = debounceMs;
          te.enabled = true;
          te.label = label;
          te.persistent = true;
          found = true;
          break;
        }
      }
      if (!found && config.touchPins.size() < MAX_TOUCH_PINS) {
        TouchEntry te;
        te.pin = pin;
        te.threshold = threshold;
        te.debounceMs = debounceMs;
        te.enabled = true;
        te.label = label;
        te.persistent = true;
        config.touchPins.push_back(te);
      }
      config.markDirty();
    }

    JsonDocument resp;
    resp["type"] = "touch_added";
    resp["deviceId"] = getDeviceId();
    resp["pin"] = pin;
    resp["threshold"] = threshold;
    resp["label"] = label;
    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
      MqttClient::publish(config.pubTopics[0].topic, p);

    // ---- remove: 移除触摸引脚 ----
  } else if (action == "remove") {
    int pin = doc["pin"] | -1;
    if (pin < 0) {
      Serial.println("[TOUCH] remove: pin required");
      return;
    }

    for (int i = 0; i < tsCount; i++) {
      if (touchStates[i].pin == pin) {
        for (int j = i; j < tsCount - 1; j++) {
          touchStates[j] = touchStates[j + 1];
        }
        tsCount--;
        Serial.printf("[TOUCH] Removed pin=%d\n", pin);
        break;
      }
    }
    for (auto it = config.touchPins.begin(); it != config.touchPins.end(); ++it) {
      if (it->pin == pin) {
        bool wasPersistent = it->persistent;
        config.touchPins.erase(it);
        if (wasPersistent) config.markDirty();
        break;
      }
    }

    // ---- read: 读取当前触摸�?----
  } else if (action == "read") {
    int pin = doc["pin"] | -1;
    if (pin < 0) {
      Serial.println("[TOUCH] read: pin required");
      return;
    }

    int raw = touchRead(pin);
    TouchState *ts = findTouchState(pin);

    JsonDocument resp;
    resp["type"] = "touch_read";
    resp["deviceId"] = getDeviceId();
    resp["pin"] = pin;
    resp["raw"] = raw;
    if (ts) {
      resp["threshold"] = ts->threshold;
      resp["touched"] = ts->touched;
      resp["label"] = ts->label;
    }
        if (doc.containsKey("result_var")) {
        String varName = doc["result_var"].as<String>();
        if (varName.length() > 0)
            ScriptEngine::setVar(varName,(float)raw, false);
    }

    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
      MqttClient::publish(config.pubTopics[0].topic, p);
    Serial.printf("[TOUCH] Read pin=%d raw=%d\n", pin, raw);

    // ---- calibrate: 校准阈�?----
  } else if (action == "calibrate") {
    int pin = doc["pin"] | -1;
    int samples = doc["samples"] | 100;
    if (pin < 0) {
      Serial.println("[TOUCH] calibrate: pin required");
      return;
    }

    const PinCapability *cap = findPinCap(pin);
    if (!cap || !cap->touch) {
      Serial.printf("[TOUCH] Pin %d does not support touch\n", pin);
      return;
    }

    long sum = 0;
    for (int i = 0; i < samples; i++) {
      sum += touchRead(pin);
      delay(5);
    }
    int avg = sum / samples;
    int thresh = avg * 70 / 100;

    // 更新运行时状�?
    TouchState *ts = findTouchState(pin);
    if (ts) ts->threshold = thresh;

    // 更新 NVS
    for (auto &te : config.touchPins) {
      if (te.pin == pin) {
        te.threshold = thresh;
        config.markDirty();
        break;
      }
    }

    JsonDocument resp;
    resp["type"] = "touch_calibrate";
    resp["deviceId"] = getDeviceId();
    resp["pin"] = pin;
    resp["baseline"] = avg;
    resp["threshold"] = thresh;
    resp["samples"] = samples;
    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
      MqttClient::publish(config.pubTopics[0].topic, p);
    Serial.printf("[TOUCH] Calibrated pin=%d: baseline=%d threshold=%d (%d samples)\n",
                  pin, avg, thresh, samples);

    // ---- enable / disable ----
  } else if (action == "enable" || action == "disable") {
    int pin = doc["pin"] | -1;
    bool en = (action == "enable");
    if (doc.containsKey("enabled")) en = doc["enabled"].as<bool>();

    TouchState *ts = findTouchState(pin);
    if (ts) {
      ts->enabled = en;
      if (en) {
        ts->prevTouched = false;
        ts->lastChange = millis();
      }
    }
    for (auto &te : config.touchPins) {
      if (te.pin == pin) {
        te.enabled = en;
        if (te.persistent) config.markDirty();
        break;
      }
    }
    Serial.printf("[TOUCH] Pin %d %s\n", pin, en ? "enabled" : "disabled");

    // ---- list: 列出所有触摸引�?----
  } else if (action == "list") {
    JsonDocument resp;
    resp["type"] = "touch_list";
    resp["deviceId"] = getDeviceId();
    JsonArray arr = resp["pins"].to<JsonArray>();
    for (int i = 0; i < tsCount; i++) {
      JsonObject o = arr.add<JsonObject>();
      o["pin"] = touchStates[i].pin;
      o["threshold"] = touchStates[i].threshold;
      o["debounceMs"] = touchStates[i].debounceMs;
      o["enabled"] = touchStates[i].enabled;
      o["label"] = touchStates[i].label;
      o["touched"] = touchStates[i].touched;
      o["rawValue"] = touchStates[i].rawValue;
    }
    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
      MqttClient::publish(config.pubTopics[0].topic, p);
    Serial.println("[TOUCH] List published");

    // ---- read_all: 一次性读取所有引�?----
  } else if (action == "read_all") {
    JsonDocument resp;
    resp["type"] = "touch_read_all";
    resp["deviceId"] = getDeviceId();
    JsonArray arr = resp["pins"].to<JsonArray>();
    for (int i = 0; i < tsCount; i++) {
      int raw = touchRead(touchStates[i].pin);
      touchStates[i].rawValue = raw;
      JsonObject o = arr.add<JsonObject>();
      o["pin"] = touchStates[i].pin;
      o["raw"] = raw;
      o["threshold"] = touchStates[i].threshold;
      o["touched"] = (raw < touchStates[i].threshold);
      o["label"] = touchStates[i].label;
    }
    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
      MqttClient::publish(config.pubTopics[0].topic, p);

  } else {
    Serial.printf("[TOUCH] Unknown action: %s\n", action.c_str());
  }
}

}  // namespace Touch
