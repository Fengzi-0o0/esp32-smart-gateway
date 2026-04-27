#include "rand_engine.h"
#include "config.h"
#include "mqtt_client.h"

static bool seeded = false;

// 引用外部命令执行
extern void executeCommand(JsonDocument &doc);

namespace Rand {

void init() {
    randomSeed(esp_random());
    seeded = true;
    Serial.println("[RAND] Engine initialized");
}

void loop() { /* 按需扩展 */ }

int getInt(int minVal, int maxVal) {
    if (maxVal <= minVal) return minVal;
    return random(minVal, maxVal + 1);
}

float getFloat(float minVal, float maxVal) {
    if (maxVal <= minVal) return minVal;
    float r = (float)esp_random() / (float)UINT32_MAX;
    return minVal + r * (maxVal - minVal);
}

bool getBool() {
    return (esp_random() % 2) == 0;
}

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();

    if (action == "int") {
        int minV = doc["min"] | 0;
        int maxV = doc["max"] | 100;
        int val  = getInt(minV, maxV);
        Serial.printf("[RAND] int [%d, %d] = %d\n", minV, maxV, val);
        JsonDocument resp;
        resp["type"]     = "rand_int";
        resp["deviceId"] = getDeviceId();
        resp["min"]      = minV;
        resp["max"]      = maxV;
        resp["value"]    = val;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "float") {
        float minV = doc["min"] | 0.0f;
        float maxV = doc["max"] | 1.0f;
        float val  = getFloat(minV, maxV);
        Serial.printf("[RAND] float [%.2f, %.2f] = %.4f\n", minV, maxV, val);
        JsonDocument resp;
        resp["type"]     = "rand_float";
        resp["deviceId"] = getDeviceId();
        resp["min"]      = minV;
        resp["max"]      = maxV;
        resp["value"]    = val;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "bool") {
        bool val = getBool();
        Serial.printf("[RAND] bool = %s\n", val ? "true" : "false");
        JsonDocument resp;
        resp["type"]     = "rand_bool";
        resp["deviceId"] = getDeviceId();
        resp["value"]    = val;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "exec") {
        int minV = doc["min"] | 0;
        int maxV = doc["max"] | 255;
        int val = getInt(minV, maxV);

        if (doc.containsKey("template")) {
            // 先把 template 序列化成字符串
            String tmplStr;
            serializeJson(doc["template"], tmplStr);

            // 替换所有 "$R" 为随机数
            tmplStr.replace("\"$R\"", String(val));

            // 重新解析并执行
            JsonDocument execDoc;
            if (!deserializeJson(execDoc, tmplStr)) {
                Serial.printf("[RAND] exec: value=%d\n", val);
                executeCommand(execDoc);
            }
        }

    } else if (action == "seed") {
        uint32_t s = doc["value"] | (uint32_t)esp_random();
        randomSeed(s);
        Serial.printf("[RAND] Seeded with %u\n", s);

    } else {
        Serial.printf("[RAND] Unknown action: %s\n", action.c_str());
    }
}

} // namespace Rand
