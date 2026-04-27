#include "timer_engine.h"
#include "config.h"
#include <ArduinoJson.h>

static std::vector<TimerTask> timers;

// 引用外部命令执行回调（在 mqtt_client.cpp 中实现）
extern void executeCommand(JsonDocument &doc);

namespace TimerEngine {

void init() {
  timers.clear();
  Serial.println("[TIMER] Engine initialized");
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < (int)timers.size(); i++) {
    TimerTask &t = timers[i];
    if (!t.enabled) continue;

    // 检查运行时长限制
    if (t.duration > 0 && (now - t.startTime >= t.duration)) {
      t.enabled = false;
      Serial.printf("[TIMER] '%s' duration expired (%lu ms)\n", t.id.c_str(), t.duration);
      if (t.autoDelete) {
        Serial.printf("[TIMER] '%s' auto-deleted (duration)\n", t.id.c_str());
        timers.erase(timers.begin() + i);
        // 同步删除 NVS
        for (auto it = config.timers.begin(); it != config.timers.end(); ++it) {
          if (it->id == t.id) {
            config.timers.erase(it);
            config.save();
            break;
          }
        }
        i--;
      }
      continue;
    }

    // 检查执行次数
    if (t.type == "once" && t.executed >= 1) continue;
    if (t.type == "count" && t.count > 0 && t.executed >= t.count) {
      t.enabled = false;
      Serial.printf("[TIMER] '%s' completed %d times\n", t.id.c_str(), t.executed);
      if (t.autoDelete) {
        Serial.printf("[TIMER] '%s' auto-deleted (count)\n", t.id.c_str());
        timers.erase(timers.begin() + i);
        for (auto it = config.timers.begin(); it != config.timers.end(); ++it) {
          if (it->id == t.id) {
            config.timers.erase(it);
            config.save();
            break;
          }
        }
        i--;
      }
      continue;
    }

    if (now - t.lastRun >= t.interval) {
      t.lastRun = now;
      t.executed++;

      // 解析并执行 commandsJson 中的命令
      JsonDocument cmdDoc;
      DeserializationError err = deserializeJson(cmdDoc, t.commandsJson);
      if (!err) {
        if (cmdDoc.is<JsonArray>()) {
          for (JsonObject c : cmdDoc.as<JsonArray>()) {
            JsonDocument tmp;
            tmp.set(c);
            Serial.printf("[TIMER] Exec '%s' cmd: %s\n",
                          t.id.c_str(), c["cmd"].as<String>().c_str());
            executeCommand(tmp);
          }
        } else if (cmdDoc.is<JsonObject>()) {
          Serial.printf("[TIMER] Exec '%s' cmd: %s\n",
                        t.id.c_str(), cmdDoc["cmd"].as<String>().c_str());
          executeCommand(cmdDoc);
        }
      } else {
        Serial.printf("[TIMER] JSON parse error for '%s': %s\n",
                      t.id.c_str(), err.c_str());
      }

      Serial.printf("[TIMER] '%s' executed %d times\n",
                    t.id.c_str(), t.executed);
    }
  }
}

void add(const TimerTask &t) {
  // 先检查是否已存在同 ID
  for (auto &existing : timers) {
    if (existing.id == t.id) {
      existing = t;
      existing.lastRun = millis();
      existing.startTime = millis();
      existing.executed = 0;

      Serial.printf("[TIMER] Updated '%s'\n", t.id.c_str());
      return;
    }
  }
  if (timers.size() < MAX_TIMERS) {
    TimerTask nt = t;
    nt.lastRun = millis();
    nt.startTime = millis();
    timers.push_back(nt);
    nt.executed = 0;
    Serial.printf("[TIMER] Added '%s' type=%s interval=%lu duration=%lu autoDelete=%d\n",
                  t.id.c_str(), t.type.c_str(), t.interval, t.duration, t.autoDelete);
  } else {
    Serial.println("[TIMER] Max timers reached");
  }
}

bool remove(const String &id) {
  for (auto it = timers.begin(); it != timers.end(); ++it) {
    if (it->id == id) {
      timers.erase(it);
      Serial.printf("[TIMER] Removed '%s'\n", id.c_str());
      return true;
    }
  }
  return false;
}

void enable(const String &id, bool en) {
  for (auto &t : timers) {
    if (t.id == id) {
      t.enabled = en;
      if (en) {
        t.lastRun = millis();
        t.startTime = millis();
        t.executed = 0;
      }
      Serial.printf("[TIMER] '%s' %s\n", id.c_str(), en ? "enabled" : "disabled");
      return;
    }
  }
}

void reset(const String &id) {
  for (auto &t : timers) {
    if (t.id == id) {
      t.executed = 0;
      t.lastRun = millis();
      t.startTime = millis();
      t.enabled = true;
      Serial.printf("[TIMER] '%s' reset\n", id.c_str());
      return;
    }
  }
}

std::vector<TimerTask> &getList() {
  return timers;
}

String toJson() {
  JsonDocument doc;
  doc["type"] = "timer_list";
  JsonArray arr = doc["timers"].to<JsonArray>();
  for (auto &t : timers) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = t.id;
    o["type"] = t.type;
    o["interval"] = t.interval;
    o["count"] = t.count;
    o["executed"] = t.executed;
    o["enabled"] = t.enabled;
    o["duration"] = t.duration;
    o["autoDelete"] = t.autoDelete;
    // 解析 commandsJson 以验证格式
    JsonDocument cmds;
    if (!deserializeJson(cmds, t.commandsJson)) {
      o["commands"] = cmds;
    }
  }
  String json;
  serializeJson(doc, json);
  return json;
}

}  // namespace TimerEngine
