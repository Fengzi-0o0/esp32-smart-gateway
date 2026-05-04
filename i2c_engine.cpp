#include "i2c_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include <Wire.h>
#include "script_engine.h"
#include "display_engine.h"  


static bool wireInitialized = false;
static int  currentSda = -1;
static int  currentScl = -1;

static bool ensureWire(int sda, int scl) {
    // ========== 新增：引脚冲突检查 ==========
    if (DisplayEngine::displaySda >= 0 &&
        (sda != DisplayEngine::displaySda || scl != DisplayEngine::displayScl)) {
        Serial.printf("[I2C] REJECTED: sda=%d scl=%d conflicts with display (sda=%d scl=%d)\n",
                      sda, scl, DisplayEngine::displaySda, DisplayEngine::displayScl);
        return false;
    }

    if (wireInitialized && currentSda == sda && currentScl == scl) return true;
    if (wireInitialized) Wire.end();
    Wire.begin(sda, scl);
    Wire.setClock(400000);  // ← 从 100kHz 改为 400kHz，与显示引擎一致
    currentSda = sda;
    currentScl = scl;
    wireInitialized = true;
    Serial.printf("[I2C] Init SDA=%d SCL=%d @400kHz\n", sda, scl);
    return true;
}


static void publishI2CResult(const String &type, int sda, int scl,
                              uint8_t addr, uint8_t reg,
                              const uint8_t *data, int len) {
    JsonDocument doc;
    doc["type"]     = type;
    doc["deviceId"] = getDeviceId();
    doc["sda"]      = sda;
    doc["scl"]      = scl;
    doc["address"]  = addr;
    if (reg != 0xFF) doc["register"] = reg;
    if (data && len > 0) {
        JsonArray arr = doc["data"].to<JsonArray>();
        for (int i = 0; i < len; i++) arr.add(data[i]);
        doc["length"] = len;
    }
    String p;
    serializeJson(doc, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0) {
        MqttClient::publish(config.pubTopics[0].topic, p);
    }
}

bool I2C::scanDevices(int sda, int scl, std::vector<uint8_t> &found) {
    ensureWire(sda, scl);
    found.clear();
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) found.push_back(addr);
    }
    return !found.empty();
}

namespace I2C {

void init() {
    wireInitialized = false;
    currentSda = -1;
    currentScl = -1;
    Serial.println("[I2C] Engine initialized");
}

void loop() { /* 按需扩展 */ }

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();
    int sda = doc["sda"] | -1;
    int scl = doc["scl"] | -1;

     if (sda < 0 && DisplayEngine::displaySda >= 0) sda = DisplayEngine::displaySda;
    if (scl < 0 && DisplayEngine::displayScl >= 0) scl = DisplayEngine::displayScl;

    if (action == "scan") {
        if (sda < 0 || scl < 0) {
            Serial.println("[I2C] scan: sda/scl required"); return;
        }
        std::vector<uint8_t> found;
        scanDevices(sda, scl, found);

        JsonDocument resp;
        resp["type"]     = "i2c_scan";
        resp["deviceId"] = getDeviceId();
        resp["sda"]      = sda;
        resp["scl"]      = scl;
        JsonArray devs = resp["devices"].to<JsonArray>();
        for (auto a : found) devs.add(a);
        resp["count"] = found.size();
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.printf("[I2C] Scan: %d devices\n", found.size());

    } else if (action == "write") {
        if (sda < 0 || scl < 0) { Serial.println("[I2C] write: sda/scl required"); return; }
        uint8_t addr = doc["address"] | 0;
        uint8_t reg  = doc["register"] | 0xFF;
        ensureWire(sda, scl);
        Wire.beginTransmission(addr);
        if (reg != 0xFF) Wire.write(reg);
        if (doc.containsKey("data")) {
            for (JsonVariant v : doc["data"].as<JsonArray>()) Wire.write((uint8_t)(v.as<int>()));
        }
        uint8_t err = Wire.endTransmission();
        Serial.printf("[I2C] Write addr=0x%02X reg=0x%02X err=%d\n", addr, reg, err);
        publishI2CResult("i2c_write_result", sda, scl, addr, reg, nullptr, 0);

    } else if (action == "read") {
        if (sda < 0 || scl < 0) { Serial.println("[I2C] read: sda/scl required"); return; }
        uint8_t addr  = doc["address"] | 0;
        uint8_t reg   = doc["register"] | 0xFF;
        uint8_t count = doc["count"] | 1;
        ensureWire(sda, scl);
        if (reg != 0xFF) {
            Wire.beginTransmission(addr);
            Wire.write(reg);
            Wire.endTransmission(false);
        }
        uint8_t received = Wire.requestFrom(addr, count);
        uint8_t buf[32];
        int i = 0;
        while (Wire.available() && i < 32 && i < received) buf[i++] = Wire.read();
        Serial.printf("[I2C] Read addr=0x%02X reg=0x%02X got %d bytes\n", addr, reg, i);
                // ===== result_var =====
        if (doc.containsKey("result_var") && i > 0) {
            String varName = doc["result_var"].as<String>();
            if (varName.length() > 0)
                ScriptEngine::setVar(varName, (float)buf[0], false);
        }
        // ===== result_var 结束 =====

        publishI2CResult("i2c_read_result", sda, scl, addr, reg, buf, i);

    } else {
        Serial.printf("[I2C] Unknown action: %s\n", action.c_str());
    }
}

} // namespace I2C
