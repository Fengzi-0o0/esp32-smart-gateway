#include "logic_engine.h"
#include <ArduinoJson.h>
#include "script_engine.h"

static std::vector<LogicRule> rules;

// 引用外部命令执行回调（在 mqtt_client.cpp 中实现）
extern void executeCommand(JsonDocument &doc);

// 评估单个条件
// logic_engine.cpp 中的 evaluateCondition 函数
static bool evaluateCondition(const SimpleCondition &cond) {
  if (cond.source == "sensor") {
    int raw = analogRead(cond.pin);
    if (cond.op == ">") return raw > (int)cond.value;
    if (cond.op == "<") return raw < (int)cond.value;
    if (cond.op == "=") return raw == (int)cond.value;
    if (cond.op == "!=") return raw != (int)cond.value;
    return false;
  } else if (cond.source == "input") {
    int val = digitalRead(cond.pin);
    if (cond.op == "high") return val == HIGH;
    if (cond.op == "low") return val == LOW;
    return false;
  } 
  else if (cond.source == "touch") {
        int raw = touchRead(cond.pin);
        if (cond.op == "<") return raw < (int)cond.value;
        if (cond.op == ">") return raw > (int)cond.value;
        return false;
    }
  else if (cond.source == "var") {
    // 新增：支持脚本变量作为条件源
    float val = ScriptEngine::getVar(cond.source_var, 0);
    if (cond.op == ">")  return val > cond.value;
    if (cond.op == "<")  return val < cond.value;
    if (cond.op == "=")  return val == cond.value;
    if (cond.op == "!=") return val != cond.value;
    if (cond.op == ">=") return val >= cond.value;
    if (cond.op == "<=") return val <= cond.value;
    return false;
  }
  return false;
}


// 评估整条规则
static bool evaluateRule(const LogicRule &rule) {
  if (rule.conditions.empty()) return false;

  if (rule.operator_ == "or") {
    for (auto &c : rule.conditions) {
      if (evaluateCondition(c)) return true;
    }
    return false;
  } else {
    // 默认 "and"
    for (auto &c : rule.conditions) {
      if (!evaluateCondition(c)) return false;
    }
    return true;
  }
}

namespace LogicEngine {

void init() {
  rules.clear();
  Serial.println("[LOGIC] Engine initialized");
}

void loop() {
  unsigned long now = millis();

  for (auto &r : rules) {
    if (!r.enabled) continue;

    bool current = evaluateRule(r);

    // 边沿检测：从 false -> true 时触发
    if (current && !r.prevCondition) {
      if (now - r.lastTrigger >= r.cooldown) {
        r.lastTrigger = now;

        // 解析并执行 actionsJson
        JsonDocument actDoc;
        DeserializationError err = deserializeJson(actDoc, r.actionsJson);
        if (!err) {
          // logic_engine.cpp 中触发后的执行部分
          if (actDoc.is<JsonArray>()) {
            for (JsonObject a : actDoc.as<JsonArray>()) {
              // 新增：检测 if/else 结构
              if (a.containsKey("if")) {
                // 解析条件
                String src = a["if"]["source"] | String("sensor");
                int pin = a["if"]["pin"] | -1;
                String op = a["if"]["op"] | String(">");
                float val = a["if"]["value"] | 0.0f;

                bool condResult = false;
                if (src == "sensor") {
                  int raw = analogRead(pin);
                  if (op == ">") condResult = raw > (int)val;
                  if (op == "<") condResult = raw < (int)val;
                  if (op == "=") condResult = raw == (int)val;
                  if (op == "!=") condResult = raw != (int)val;
                } else if (src == "input") {
                  int v = digitalRead(pin);
                  if (op == "high") condResult = v == HIGH;
                  if (op == "low") condResult == LOW;
                }

                // 执行 then 或 else 分支
                JsonArray branch = condResult ? a["then"].as<JsonArray>()
                                              : a["else"].as<JsonArray>();
                if (!branch.isNull()) {
                  for (JsonObject b : branch) {
                    JsonDocument tmp;
                    tmp.set(b);
                    executeCommand(tmp);
                  }
                }
              } else {
                // 原有逻辑：普通命令直接执行
                JsonDocument tmp;
                tmp.set(a);
                executeCommand(tmp);
              }
            }
          }

          else if (actDoc.is<JsonObject>()) {
            Serial.printf("[LOGIC] Rule '%s' exec: %s\n",
                          r.id.c_str(), actDoc["cmd"].as<String>().c_str());
            executeCommand(actDoc);
          }
        }
      }
    }
    r.prevCondition = current;
  }
}

void add(const LogicRule &r) {
  for (auto &existing : rules) {
    if (existing.id == r.id) {
      existing = r;
      existing.prevCondition = false;
      existing.lastTrigger = 0;
      Serial.printf("[LOGIC] Updated rule '%s'\n", r.id.c_str());
      return;
    }
  }
  if (rules.size() < MAX_LOGIC_RULES) {
    LogicRule nr = r;
    nr.prevCondition = false;
    nr.lastTrigger = 0;
    rules.push_back(nr);
    Serial.printf("[LOGIC] Added rule '%s' op=%s conditions=%d\n",
                  r.id.c_str(), r.operator_.c_str(), r.conditions.size());
  } else {
    Serial.println("[LOGIC] Max rules reached");
  }
}

bool remove(const String &id) {
  for (auto it = rules.begin(); it != rules.end(); ++it) {
    if (it->id == id) {
      rules.erase(it);
      Serial.printf("[LOGIC] Removed rule '%s'\n", id.c_str());
      return true;
    }
  }
  return false;
}

void enable(const String &id, bool en) {
  for (auto &r : rules) {
    if (r.id == id) {
      r.enabled = en;
      if (en) {
        r.prevCondition = false;
        r.lastTrigger = 0;
      }
      Serial.printf("[LOGIC] Rule '%s' %s\n", id.c_str(), en ? "enabled" : "disabled");
      return;
    }
  }
}

std::vector<LogicRule> &getList() {
  return rules;
}

String toJson() {
  JsonDocument doc;
  doc["type"] = "logic_list";
  JsonArray arr = doc["rules"].to<JsonArray>();
  for (auto &r : rules) {
    JsonObject o = arr.add<JsonObject>();
    o["id"] = r.id;
    o["operator"] = r.operator_;
    o["enabled"] = r.enabled;
    o["cooldown"] = r.cooldown;

    JsonArray conds = o["conditions"].to<JsonArray>();
    for (auto &c : r.conditions) {
      JsonObject co = conds.add<JsonObject>();
      co["source"] = c.source;
      co["pin"] = c.pin;
      co["op"] = c.op;
      co["value"] = c.value;
    }

    // 解析 actionsJson 以验证格式
    JsonDocument acts;
    if (!deserializeJson(acts, r.actionsJson)) {
      o["actions"] = acts;
    }
  }
  String json;
  serializeJson(doc, json);
  return json;
}

}  // namespace LogicEngine
