#include "onewire_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include "script_engine.h"

// ========== 1-Wire 底层时序 (Arduino API, ESP32-S3 @240MHz) ==========

static void delayUs(uint32_t us) {
    if (us > 1000) { delay(us / 1000); us = us % 1000; }
    if (us > 0) delayMicroseconds(us);
}

static void owDriveLow(uint8_t pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

static void owRelease(uint8_t pin) {
    pinMode(pin, INPUT_PULLUP);
}

static int owReadPin(uint8_t pin) {
    return digitalRead(pin);
}

// ========== 基本操作 ==========

static bool owReset(uint8_t pin) {
    owRelease(pin);
    delayUs(500);

    owDriveLow(pin);
    delayUs(480);
    owRelease(pin);

    delayUs(70);
    bool present = (owReadPin(pin) == 0);
    delayUs(410);
    return present;
}

static void owWriteBit(uint8_t pin, int bit) {
    if (bit) {
        owDriveLow(pin);
        delayUs(6);
        owRelease(pin);
        delayUs(64);
    } else {
        owDriveLow(pin);
        delayUs(60);
        owRelease(pin);
        delayUs(10);
    }
}

static int owReadBit(uint8_t pin) {
    owDriveLow(pin);
    delayUs(3);
    owRelease(pin);
    delayUs(10);
    int bit = owReadPin(pin);
    delayUs(53);
    return bit;
}

static void owWriteByte(uint8_t pin, uint8_t byte) {
    for (int i = 0; i < 8; i++) owWriteBit(pin, (byte >> i) & 1);
}

static uint8_t owReadByte(uint8_t pin) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (owReadBit(pin)) byte |= (1 << i);
    }
    return byte;
}

// ========== ROM 搜索算法 ==========

static void owSearchDevices(uint8_t pin, std::vector<uint64_t> &roms) {
    roms.clear();
    uint8_t lastDiscrepancy = 0;
    uint8_t rom[8];
    bool done = false;

    while (!done && roms.size() < 10) {
        if (!owReset(pin)) break;

        owWriteByte(pin, 0xF0); // Search ROM
        uint8_t discrepancy = 0;
        uint8_t lastZero = 0;
        bool found = true;

        memset(rom, 0, sizeof(rom));

        for (int i = 0; i < 64; i++) {
            int byteIdx = i / 8;
            int bitIdx  = i % 8;
            int a = owReadBit(pin);
            int b = owReadBit(pin);

            if (a == 1 && b == 1) { found = false; break; }

            int bit;
            if (a == 0 && b == 0) {
                if (i < lastDiscrepancy) {
                    bit = (rom[byteIdx] >> bitIdx) & 1;
                } else if (i == lastDiscrepancy) {
                    bit = 1;
                } else {
                    bit = 0;
                }
                if (bit == 0) {
                    lastZero = i;
                    discrepancy = i;
                }
            } else {
                bit = a;
            }

            if (bit) rom[byteIdx] |= (1 << bitIdx);
            owWriteBit(pin, bit);
        }

        if (found) {
            uint64_t id = 0;
            for (int i = 0; i < 8; i++) id |= ((uint64_t)rom[i] << (i * 8));
            roms.push_back(id);
        }

        lastDiscrepancy = discrepancy;
        if (lastDiscrepancy == 0) done = true;
    }
}

// ========== DS18B20 温度读取 ==========

static float readDS18B20(uint8_t pin, uint64_t rom) {
    if (!owReset(pin)) return -999.0f;

    if (rom != 0) {
        owWriteByte(pin, 0x55); // Match ROM
        for (int i = 0; i < 8; i++) owWriteByte(pin, (rom >> (i * 8)) & 0xFF);
    } else {
        owWriteByte(pin, 0xCC); // Skip ROM
    }

    owWriteByte(pin, 0x44); // Convert T
    owRelease(pin);

    unsigned long t = millis();
    while (owReadPin(pin) == 0) {
        if (millis() - t > 1000) return -999.0f;
        delay(10);
    }

    if (!owReset(pin)) return -999.0f;

    if (rom != 0) {
        owWriteByte(pin, 0x55);
        for (int i = 0; i < 8; i++) owWriteByte(pin, (rom >> (i * 8)) & 0xFF);
    } else {
        owWriteByte(pin, 0xCC);
    }

    owWriteByte(pin, 0xBE); // Read Scratchpad

    uint8_t lo = owReadByte(pin);
    uint8_t hi = owReadByte(pin);
    int16_t raw = (hi << 8) | lo;
    return raw / 16.0f;
}

// ========== 命令处理 ==========

namespace OneWire {

void init() { Serial.println("[OW] Engine initialized"); }
void loop() { /* 按需扩展 */ }

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();
    int pin = doc["pin"] | -1;
    if (pin < 0) { Serial.println("[OW] pin required"); return; }

    if (action == "reset") {
        bool present = owReset((uint8_t)pin);
        Serial.printf("[OW] Reset pin=%d present=%s\n", pin, present ? "YES" : "NO");
        JsonDocument resp;
        resp["type"]     = "ow_reset";
        resp["deviceId"] = getDeviceId();
        resp["pin"]      = pin;
        resp["present"]  = present;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "search") {
        std::vector<uint64_t> roms;
        owSearchDevices((uint8_t)pin, roms);
        JsonDocument resp;
        resp["type"]     = "ow_search";
        resp["deviceId"] = getDeviceId();
        resp["pin"]      = pin;
        resp["count"]    = roms.size();
        JsonArray devs = resp["devices"].to<JsonArray>();
        for (auto &r : roms) {
            char buf[20];
            snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)r);
            devs.add(String(buf));
        }
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.printf("[OW] Search pin=%d found %d devices\n", pin, roms.size());

    } else if (action == "read_temp") {
        uint64_t rom = 0;
        if (doc.containsKey("rom")) {
            rom = strtoull(doc["rom"].as<String>().c_str(), NULL, 16);
        }
        float temp = readDS18B20((uint8_t)pin, rom);
        Serial.printf("[OW] Temp pin=%d %.2f C\n", pin, temp);
        JsonDocument resp;
        resp["type"]        = "ow_temp";
        resp["deviceId"]    = getDeviceId();
        resp["pin"]         = pin;
        resp["temperature"] = temp;
            if (doc.containsKey("result_var")) {
        String varName = doc["result_var"].as<String>();
        if (varName.length() > 0)
            ScriptEngine::setVar(varName,temp, false);
    }

        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "write_byte") {
        uint8_t byte = doc["value"] | 0;
        owReset((uint8_t)pin);
        owWriteByte((uint8_t)pin, byte);
        Serial.printf("[OW] WriteByte pin=%d 0x%02X\n", pin, byte);

    } else if (action == "read_byte") {
        uint8_t byte = owReadByte((uint8_t)pin);
        Serial.printf("[OW] ReadByte pin=%d 0x%02X\n", pin, byte);
        JsonDocument resp;
        resp["type"]     = "ow_read_byte";
        resp["deviceId"] = getDeviceId();
        resp["pin"]      = pin;
        resp["value"]    = byte;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "read_bytes") {
        uint8_t count = doc["count"] | 1;
        if (count > 32) count = 32;
        JsonDocument resp;
        resp["type"]     = "ow_read_bytes";
        resp["deviceId"] = getDeviceId();
        resp["pin"]      = pin;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < count; i++) arr.add(owReadByte((uint8_t)pin));
        resp["length"] = count;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else {
        Serial.printf("[OW] Unknown action: %s\n", action.c_str());
    }
}

} // namespace OneWire
