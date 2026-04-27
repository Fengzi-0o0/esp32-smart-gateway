#include "script_engine.h"
#include "mqtt_client.h"
#include "config.h"
#include "timer_engine.h"
#include "logic_engine.h"
#include "gpio_control.h"
#include <ArduinoJson.h>

#define MAX_SCRIPT_VARS      20
#define MAX_SCRIPT_SEQUENCES 5
#define MAX_SCRIPT_LOOPS     5
#define MAX_EXEC_DEPTH       3

static Variable vars[MAX_SCRIPT_VARS];
static int varCount = 0;

static SequenceState sequences[MAX_SCRIPT_SEQUENCES];
static int seqCount = 0;

static LoopState loops[MAX_SCRIPT_LOOPS];
static int loopCount = 0;

static int execDepth = 0;

extern void executeCommand(JsonDocument &doc);

// Forward declaration
static void interpolateVars(JsonDocument &doc);
static void processCommands(JsonArray cmds, const String &defaultTarget);

// ================================================================
//  Variable management
// ================================================================

static Variable* findVar(const String &name) {
    for (int i = 0; i < varCount; i++) {
        if (vars[i].name == name) return &vars[i];
    }
    return nullptr;
}

float ScriptEngine::getVar(const String &name, float defaultVal) {
    Variable *v = findVar(name);
    return v ? v->value : defaultVal;
}

void ScriptEngine::setVar(const String &name, float value, bool persistent) {
    Variable *v = findVar(name);
    if (v) {
        v->value = value;
        v->persistent = persistent;
        Serial.printf("[SCRIPT] Var '%s' = %.2f\n", name.c_str(), value);
        return;
    }
    if (varCount < MAX_SCRIPT_VARS) {
        vars[varCount].name = name;
        vars[varCount].value = value;
        vars[varCount].persistent = persistent;
        varCount++;
        Serial.printf("[SCRIPT] Var '%s' = %.2f (new)\n", name.c_str(), value);
    } else {
        Serial.println("[SCRIPT] Max vars reached");
    }
}

bool ScriptEngine::removeVar(const String &name) {
    for (int i = 0; i < varCount; i++) {
        if (vars[i].name == name) {
            for (int j = i; j < varCount - 1; j++) {
                vars[j] = vars[j + 1];
            }
            varCount--;
            Serial.printf("[SCRIPT] Var '%s' removed\n", name.c_str());
            return true;
        }
    }
    return false;
}

void ScriptEngine::clearVars() {
    varCount = 0;
    Serial.println("[SCRIPT] All vars cleared");
}

float ScriptEngine::incrementVar(const String &name, float step) {
    Variable *v = findVar(name);
    if (v) {
        v->value += step;
        Serial.printf("[SCRIPT] Var '%s' += %.2f -> %.2f\n", name.c_str(), step, v->value);
        return v->value;
    }
    setVar(name, step, false);
    return step;
}

// ================================================================
//  Variable interpolation: replace $V:varname in JSON strings
// ================================================================

static String extractVarName(const String &s, int start) {
    String varName = "";
    for (int i = start; i < (int)s.length(); i++) {
        char c = s.charAt(i);
        if (isAlphaNumeric(c) || c == '_') {
            varName += c;
        } else {
            break;
        }
    }
    return varName;
}

static bool interpolateString(String &s) {
    bool changed = false;
    int searchFrom = 0;
    while (true) {
        int pos = s.indexOf("$V:", searchFrom);
        if (pos < 0) break;
        String varName = extractVarName(s, pos + 3);
        if (varName.length() > 0) {
            float val = ScriptEngine::getVar(varName, 0);
            s = s.substring(0, pos) + String((int)val) + s.substring(pos + 3 + varName.length());
            changed = true;
            searchFrom = pos + String((int)val).length();
        } else {
            searchFrom = pos + 3;
        }
    }
    return changed;
}

static void interpolateVars(JsonDocument &doc) {
    if (doc.is<JsonObject>()) {
        for (JsonPair kv : doc.as<JsonObject>()) {
            JsonVariant val = kv.value();
            if (val.is<String>()) {
                String s = val.as<String>();
                if (interpolateString(s)) {
                    kv.value().set(s);
                }
            } else if (val.is<JsonObject>() || val.is<JsonArray>()) {
                JsonDocument sub;
                sub.set(val);
                interpolateVars(sub);
                // Note: nested interpolation limited by ArduinoJson copy semantics
            }
        }
    } else if (doc.is<JsonArray>()) {
        for (JsonVariant v : doc.as<JsonArray>()) {
            if (v.is<String>()) {
                String s = v.as<String>();
                if (interpolateString(s)) {
                    v.set(s);
                }
            }
        }
    }
}

// ================================================================
//  Sequence management (non-blocking)
// ================================================================

static int findSequence(const String &id) {
    for (int i = 0; i < seqCount; i++) {
        if (sequences[i].id == id) return i;
    }
    return -1;
}

void ScriptEngine::addSequence(const String &id, const String &cmdsJson,
                                unsigned long stepDelay, const String &target) {
    int idx = findSequence(id);
    if (idx < 0) {
        for (int i = 0; i < seqCount; i++) {
            if (!sequences[i].active) { idx = i; break; }
        }
    }
    if (idx < 0 && seqCount < MAX_SCRIPT_SEQUENCES) {
        idx = seqCount++;
    }
    if (idx < 0) {
        Serial.println("[SCRIPT] Max sequences reached");
        return;
    }

    SequenceState &ss = sequences[idx];
    ss.id = id;
    ss.commandsJson = cmdsJson;
    ss.step = 0;
    ss.lastStep = millis();
    ss.stepDelay = stepDelay;
    ss.active = true;
    ss.target = target;

    JsonDocument tmp;
    if (!deserializeJson(tmp, cmdsJson) && tmp.is<JsonArray>()) {
        ss.total = tmp.as<JsonArray>().size();
    } else {
        ss.total = 0;
    }

    Serial.printf("[SCRIPT] Sequence '%s' steps=%d delay=%lu target=%s\n",
                  id.c_str(), ss.total, stepDelay, target.c_str());
}

static void processSequences() {
    unsigned long now = millis();
    for (int i = 0; i < seqCount; i++) {
        SequenceState &ss = sequences[i];
        if (!ss.active) continue;

        if (ss.step >= ss.total) {
            ss.active = false;
            Serial.printf("[SCRIPT] Sequence '%s' completed\n", ss.id.c_str());
            continue;
        }

        if (now - ss.lastStep < ss.stepDelay) continue;
        ss.lastStep = now;

        JsonDocument cmdsDoc;
        if (deserializeJson(cmdsDoc, ss.commandsJson)) continue;

        JsonArray arr = cmdsDoc.as<JsonArray>();
        if (ss.step >= (int)arr.size()) {
            ss.active = false;
            continue;
        }

        JsonDocument cmdDoc;
        cmdDoc.set(arr[ss.step]);

        if (ss.target.length() > 0 && !cmdDoc.containsKey("target")) {
            cmdDoc["target"] = ss.target;
        }

        interpolateVars(cmdDoc);
        executeCommand(cmdDoc);

        Serial.printf("[SCRIPT] Seq '%s' step %d/%d\n",
                      ss.id.c_str(), ss.step + 1, ss.total);
        ss.step++;
    }
}

// ================================================================
//  Loop management (non-blocking)
// ================================================================

static int findLoop(const String &id) {
    for (int i = 0; i < loopCount; i++) {
        if (loops[i].id == id) return i;
    }
    return -1;
}

void ScriptEngine::addLoop(const String &id, const String &cmdsJson,
                            int count, unsigned long interval, const String &target) {
    int idx = findLoop(id);
    if (idx < 0) {
        for (int i = 0; i < loopCount; i++) {
            if (!loops[i].active) { idx = i; break; }
        }
    }
    if (idx < 0 && loopCount < MAX_SCRIPT_LOOPS) {
        idx = loopCount++;
    }
    if (idx < 0) {
        Serial.println("[SCRIPT] Max loops reached");
        return;
    }

    LoopState &ls = loops[idx];
    ls.id = id;
    ls.commandsJson = cmdsJson;
    ls.count = count;
    ls.executed = 0;
    ls.interval = interval;
    ls.lastRun = millis();
    ls.active = true;
    ls.target = target;

    Serial.printf("[SCRIPT] Loop '%s' count=%d interval=%lu target=%s\n",
                  id.c_str(), count, interval, target.c_str());
}

static void processLoops() {
    unsigned long now = millis();
    for (int i = 0; i < loopCount; i++) {
        LoopState &ls = loops[i];
        if (!ls.active) continue;

        if (ls.count > 0 && ls.executed >= ls.count) {
            ls.active = false;
            Serial.printf("[SCRIPT] Loop '%s' completed %d times\n",
                          ls.id.c_str(), ls.executed);
            continue;
        }

        if (now - ls.lastRun < ls.interval) continue;
        ls.lastRun = now;
        ls.executed++;

        JsonDocument cmdsDoc;
        if (deserializeJson(cmdsDoc, ls.commandsJson)) continue;

        for (JsonObject cmd : cmdsDoc.as<JsonArray>()) {
            JsonDocument cmdDoc;
            cmdDoc.set(cmd);

            if (ls.target.length() > 0 && !cmdDoc.containsKey("target")) {
                cmdDoc["target"] = ls.target;
            }

            interpolateVars(cmdDoc);
            executeCommand(cmdDoc);
        }

        Serial.printf("[SCRIPT] Loop '%s' exec %d/%s\n",
                      ls.id.c_str(), ls.executed,
                      ls.count < 0 ? "inf" : String(ls.count).c_str());
    }
}

// ================================================================
//  Condition evaluation
// ================================================================

static bool evaluateCondition(JsonObject cond) {
    String source = cond["source"] | String("var");
    String op     = cond["op"]    | String("==");
    float value   = cond["value"] | 0.0f;

    float current = 0;

    if (source == "var") {
        String name = cond["name"] | String("");
        current = ScriptEngine::getVar(name, 0);
    }
    else if (source == "sensor") {
        int pin = cond["pin"] | -1;
        if (pin < 0) return false;
        current = (float)analogRead(pin);
    }
    else if (source == "input") {
        int pin = cond["pin"] | -1;
        if (pin < 0) return false;
        current = (float)digitalRead(pin);
        if (op == "high") return current == HIGH;
        if (op == "low")  return current == LOW;
    }

    else if (source == "touch") {
        int pin = cond["pin"] | -1;
        if (pin < 0) return false;
        current = (float)touchRead(pin);
        if (op == "<")  return current < value;
        if (op == ">")  return current > value;
        if (op == "<=") return current <= value;
        if (op == ">=") return current >= value;
    }

    if (op == ">")  return current > value;
    if (op == "<")  return current < value;
    if (op == ">=") return current >= value;
    if (op == "<=") return current <= value;
    if (op == "==") return current == value;
    if (op == "!=") return current != value;

    return false;
}

// ================================================================
//  Command execution (recursive, depth-limited)
// ================================================================

static void processCommands(JsonArray cmds, const String &defaultTarget) {
    for (JsonObject cmd : cmds) {
        JsonDocument tmp;
        tmp.set(cmd);

        if (defaultTarget.length() > 0 && !tmp.containsKey("target")) {
            tmp["target"] = defaultTarget;
        }

        interpolateVars(tmp);
        ScriptEngine::handleCommand(tmp);
    }
}

// ================================================================
//  Main dispatch
// ================================================================

namespace ScriptEngine {

void init() {
    varCount = 0;
    seqCount = 0;
    loopCount = 0;
    execDepth = 0;
    Serial.println("[SCRIPT] Engine initialized");
}

void loop() {
    processSequences();
    processLoops();
}

void handleCommand(JsonDocument &doc) {
    const char *target = doc["target"];
    if (target && strlen(target) > 0) {
        String myId = getDeviceId();
        if (String(target) != "all" && String(target) != myId) return;
    }

    String action = doc["action"].as<String>();
    Serial.printf("[SCRIPT] action=%s\n", action.c_str());

    if (action == "var_set") {
        String name  = doc["name"].as<String>();
        float value  = doc["value"] | 0.0f;
        bool persist = doc["persistent"] | false;
        setVar(name, value, persist);
    }
    else if (action == "var_get") {
        String name = doc["name"].as<String>();
        JsonDocument resp;
        resp["type"]     = "script_var";
        resp["deviceId"] = getDeviceId();
        resp["name"]     = name;
        resp["value"]    = getVar(name, 0);
        Variable *v = findVar(name);
        resp["exists"]   = (v != nullptr);
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
    }
    else if (action == "var_inc") {
        String name = doc["name"].as<String>();
        float step  = doc["step"] | 1.0f;
        float val   = incrementVar(name, step);

        if (doc.containsKey("threshold")) {
            float thresh = doc["threshold"] | 0.0f;
            if (val >= thresh) {
                Serial.printf("[SCRIPT] Var '%s' reached threshold %.2f\n",
                              name.c_str(), thresh);
                if (doc.containsKey("commands")) {
                    JsonDocument cmdsDoc;
                    cmdsDoc.set(doc["commands"]);
                    interpolateVars(cmdsDoc);
                    String subTarget = doc.containsKey("target") ? doc["target"].as<String>() : "";
                    if (execDepth < MAX_EXEC_DEPTH) {
                        execDepth++;
                        processCommands(cmdsDoc.as<JsonArray>(), subTarget);
                        execDepth--;
                    }
                }
            }
        }
    }
    else if (action == "var_remove") {
        String name = doc["name"].as<String>();
        removeVar(name);
    }
    else if (action == "var_clear") {
        clearVars();
    }
    else if (action == "var_list") {
        JsonDocument resp;
        resp["type"]     = "script_vars";
        resp["deviceId"] = getDeviceId();
        JsonArray arr = resp["variables"].to<JsonArray>();
        for (int i = 0; i < varCount; i++) {
            JsonObject o = arr.add<JsonObject>();
            o["name"]       = vars[i].name;
            o["value"]      = vars[i].value;
            o["persistent"] = vars[i].persistent;
        }
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
        Serial.printf("[SCRIPT] Listed %d vars\n", varCount);
    }
    else if (action == "if") {
        if (!doc.containsKey("condition") || !doc.containsKey("then")) {
            Serial.println("[SCRIPT] if: need condition and then");
            return;
        }
        bool result = evaluateCondition(doc["condition"].as<JsonObject>());
        Serial.printf("[SCRIPT] if -> %s\n", result ? "true" : "false");

        String subTarget = doc.containsKey("target") ? doc["target"].as<String>() : "";

        if (execDepth >= MAX_EXEC_DEPTH) {
            Serial.printf("[SCRIPT] Max depth %d reached\n", MAX_EXEC_DEPTH);
            return;
        }
        execDepth++;

        if (result) {
            JsonDocument thenDoc;
            thenDoc.set(doc["then"]);
            interpolateVars(thenDoc);
            processCommands(thenDoc.as<JsonArray>(), subTarget);
        } else if (doc.containsKey("else")) {
            JsonDocument elseDoc;
            elseDoc.set(doc["else"]);
            interpolateVars(elseDoc);
            processCommands(elseDoc.as<JsonArray>(), subTarget);
        }

        execDepth--;
    }
    else if (action == "sequence") {
        String id           = doc["id"].as<String>();
        unsigned long delay = doc["stepDelay"] | 500;
        String subTarget    = doc.containsKey("target") ? doc["target"].as<String>() : "";

        if (!doc.containsKey("commands")) {
            Serial.println("[SCRIPT] sequence: need commands");
            return;
        }

        String cmdsJson;
        serializeJson(doc["commands"], cmdsJson);
        addSequence(id, cmdsJson, delay, subTarget);
    }
    else if (action == "loop") {
        String id           = doc["id"].as<String>();
        int count           = doc["count"] | -1;
        unsigned long interval = doc["interval"] | 1000;
        String subTarget    = doc.containsKey("target") ? doc["target"].as<String>() : "";

        if (!doc.containsKey("commands")) {
            Serial.println("[SCRIPT] loop: need commands");
            return;
        }

        String cmdsJson;
        serializeJson(doc["commands"], cmdsJson);
        addLoop(id, cmdsJson, count, interval, subTarget);
    }

        else if (action == "math") {
        String op = doc["op"].as<String>();
        String resultVar = doc["result"].as<String>();
        bool persist = doc["persistent"] | false;

        if (resultVar.length() == 0) {
            Serial.println("[SCRIPT] math: result var name required");
            return;
        }

        // 解析 a 的值
        float a = 0;
        if (doc["a"].is<float>() || doc["a"].is<int>()) {
            a = doc["a"].as<float>();
        } else if (doc["a"].is<JsonObject>()) {
            String src = doc["a"]["source"] | String("var");
            if (src == "sensor") {
                int pin = doc["a"]["pin"] | -1;
                if (pin >= 0) a = (float)analogRead(pin);
            } else if (src == "input") {
                int pin = doc["a"]["pin"] | -1;
                if (pin >= 0) a = (float)digitalRead(pin);
            } else if (src == "var") {
                String name = doc["a"]["name"] | String("");
                a = getVar(name, 0);
            }
        }

        // 解析 b 的值（abs/round 不需要 b）
        float b = 0;
        if (doc.containsKey("b")) {
            if (doc["b"].is<float>() || doc["b"].is<int>()) {
                b = doc["b"].as<float>();
            } else if (doc["b"].is<JsonObject>()) {
                String src = doc["b"]["source"] | String("var");
                if (src == "sensor") {
                    int pin = doc["b"]["pin"] | -1;
                    if (pin >= 0) b = (float)analogRead(pin);
                } else if (src == "input") {
                    int pin = doc["b"]["pin"] | -1;
                    if (pin >= 0) b = (float)digitalRead(pin);
                } else if (src == "var") {
                    String name = doc["b"]["name"] | String("");
                    b = getVar(name, 0);
                }
            }
        }

        // clamp 需要第三个参数
        float extra = 0;
        if (doc.containsKey("extra")) {
            extra = doc["extra"].as<float>();
        }

        // 计算
        float result = 0;
        if (op == "add") {
            result = a + b;
        } else if (op == "sub") {
            result = a - b;
        } else if (op == "mul") {
            result = a * b;
        } else if (op == "div") {
            if (b == 0) {
                Serial.printf("[SCRIPT] math: division by zero (a=%.2f)\n", a);
                return;
            }
            result = a / b;
        } else if (op == "mod") {
            if (b == 0) {
                Serial.printf("[SCRIPT] math: mod by zero\n");
                return;
            }
            result = fmod(a, b);
        } else if (op == "abs") {
            result = fabs(a);
        } else if (op == "min") {
            result = (a < b) ? a : b;
        } else if (op == "max") {
            result = (a > b) ? a : b;
        } else if (op == "clamp") {
            // clamp(a, b, extra) 即 clamp(value, min, max)
            result = a;
            if (result < b) result = b;
            if (result > extra) result = extra;
        } else if (op == "round") {
            result = round(a);
        } else if (op == "floor") {
            result = floor(a);
        } else if (op == "ceil") {
            result = ceil(a);
        } else {
            Serial.printf("[SCRIPT] math: unknown op '%s'\n", op.c_str());
            return;
        }

        setVar(resultVar, result, persist);
        Serial.printf("[SCRIPT] math %s: a=%.4f b=%.4f -> %s=%.4f\n",
                      op.c_str(), a, b, resultVar.c_str(), result);
    }


    else if (action == "exec") {
        if (execDepth >= MAX_EXEC_DEPTH) {
            Serial.printf("[SCRIPT] exec: max depth %d reached\n", MAX_EXEC_DEPTH);
            return;
        }

        if (doc.containsKey("command")) {
            JsonDocument cmdDoc;
            cmdDoc.set(doc["command"]);
            interpolateVars(cmdDoc);
            execDepth++;
            executeCommand(cmdDoc);
            execDepth--;
        }
        else if (doc.containsKey("commands")) {
            JsonDocument cmdsDoc;
            cmdsDoc.set(doc["commands"]);
            interpolateVars(cmdsDoc);
            String subTarget = doc.containsKey("target") ? doc["target"].as<String>() : "";
            execDepth++;
            processCommands(cmdsDoc.as<JsonArray>(), subTarget);
            execDepth--;
        }
    }
    else if (action == "stop_sequence") {
        String id = doc["id"].as<String>();
        int idx = findSequence(id);
        if (idx >= 0) {
            sequences[idx].active = false;
            Serial.printf("[SCRIPT] Sequence '%s' stopped\n", id.c_str());
        }
    }
    else if (action == "stop_loop") {
        String id = doc["id"].as<String>();
        int idx = findLoop(id);
        if (idx >= 0) {
            loops[idx].active = false;
            Serial.printf("[SCRIPT] Loop '%s' stopped\n", id.c_str());
        }
    }
    else if (action == "list") {
        {
            JsonDocument resp;
            resp["type"]     = "script_vars";
            resp["deviceId"] = getDeviceId();
            JsonArray arr = resp["variables"].to<JsonArray>();
            for (int i = 0; i < varCount; i++) {
                JsonObject o = arr.add<JsonObject>();
                o["name"]       = vars[i].name;
                o["value"]      = vars[i].value;
                o["persistent"] = vars[i].persistent;
            }
            String p;
            serializeJson(resp, p);
            if (MqttClient::isConnected() && config.pubTopics.size() > 0)
                MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
        }
        {
            JsonDocument resp;
            resp["type"]     = "script_sequences";
            resp["deviceId"] = getDeviceId();
            JsonArray arr = resp["sequences"].to<JsonArray>();
            for (int i = 0; i < seqCount; i++) {
                JsonObject o = arr.add<JsonObject>();
                o["id"]        = sequences[i].id;
                o["step"]      = sequences[i].step;
                o["total"]     = sequences[i].total;
                o["active"]    = sequences[i].active;
                o["stepDelay"] = sequences[i].stepDelay;
                o["target"]    = sequences[i].target;
            }
            String p;
            serializeJson(resp, p);
            if (MqttClient::isConnected() && config.pubTopics.size() > 0)
                MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
        }

        {
            JsonDocument resp;
            resp["type"]     = "script_loops";
            resp["deviceId"] = getDeviceId();
            JsonArray arr = resp["loops"].to<JsonArray>();
            for (int i = 0; i < loopCount; i++) {
                JsonObject o = arr.add<JsonObject>();
                o["id"]       = loops[i].id;
                o["count"]    = loops[i].count;
                o["executed"] = loops[i].executed;
                o["active"]   = loops[i].active;
                o["interval"] = loops[i].interval;
                o["target"]   = loops[i].target;
            }
            String p;
            serializeJson(resp, p);
            if (MqttClient::isConnected() && config.pubTopics.size() > 0)
                MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
        }
    }
    else if (action == "status") {
        JsonDocument resp;
        resp["type"]      = "script_status";
        resp["deviceId"]  = getDeviceId();
        resp["vars"]      = varCount;
        resp["sequences"] = seqCount;
        resp["loops"]     = loopCount;
        int activeSeq = 0, activeLoop = 0;
        for (int i = 0; i < seqCount; i++) if (sequences[i].active) activeSeq++;
        for (int i = 0; i < loopCount; i++) if (loops[i].active) activeLoop++;
        resp["activeSequences"] = activeSeq;
        resp["activeLoops"]     = activeLoop;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
    }
    else {
        Serial.printf("[SCRIPT] Unknown action: %s\n", action.c_str());
    }
}

} // namespace ScriptEngine
