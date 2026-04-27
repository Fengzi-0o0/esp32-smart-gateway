#include "spi_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include "pin_caps.h"
#include <SPI.h>

// ========== SPI 实例管理 ==========
// FSPI = SPI2 (port 2), HSPI = SPI3 (port 3)
// SPI0/SPI1 为 Flash/PSRAM 专用，禁止使用

static SPIClass *spi = nullptr;
static int curPort = -1, curMosi = -1, curMiso = -1, curSclk = -1;

static SPIClass* ensureSPI(int port, int mosi, int miso, int sclk) {
    if (spi && curPort == port && curMosi == mosi && curMiso == miso && curSclk == sclk) {
        return spi;
    }
    if (spi) {
        spi->end();
        delete spi;
        spi = nullptr;
    }
    int spiBus = (port == 3) ? HSPI : FSPI;
    spi = new SPIClass(spiBus);
    spi->begin(sclk, miso, mosi, -1);  // CS 由用户在命令中指定
    curPort = port; curMosi = mosi; curMiso = miso; curSclk = sclk;
    Serial.printf("[SPI] Init port=%d(FSPI/HSPI) MOSI=%d MISO=%d SCLK=%d\n",
                  port, mosi, miso, sclk);
    return spi;
}

static uint8_t spiMode(int mode) {
    switch (mode) {
        case 0: return SPI_MODE0;
        case 1: return SPI_MODE1;
        case 2: return SPI_MODE2;
        case 3: return SPI_MODE3;
        default: return SPI_MODE0;
    }
}

namespace SPIBus {

void init() {
    spi = nullptr;
    curPort = -1;
    Serial.println("[SPI] Engine initialized");
}

void loop() { /* 按需扩展 */ }

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();

    // ---- config: 配置 SPI 总线 ----
    if (action == "config") {
        int port  = doc["port"]  | 2;   // 2=FSPI, 3=HSPI
        int mosi  = doc["mosi"]  | -1;
        int miso  = doc["miso"]  | -1;
        int sclk  = doc["sclk"]  | -1;
        int speed = doc["speed"] | 1000000;
        int mode  = doc["mode"]  | 0;
        bool persist = doc["persistent"] | false;

        if (port != 2 && port != 3) {
            Serial.println("[SPI] port must be 2 (FSPI) or 3 (HSPI)");
            return;
        }
        // 引脚验证
        if (mosi >= 0 && !isValidExternalPin(mosi)) {
            Serial.printf("[SPI] REJECTED: mosi=%d\n", mosi); return;
        }
        if (miso >= 0 && !isValidExternalPin(miso)) {
            Serial.printf("[SPI] REJECTED: miso=%d\n", miso); return;
        }
        if (sclk >= 0 && !isValidExternalPin(sclk)) {
            Serial.printf("[SPI] REJECTED: sclk=%d\n", sclk); return;
        }

        ensureSPI(port, mosi, miso, sclk);

        Serial.printf("[SPI] Configured port=%d MOSI=%d MISO=%d SCLK=%d speed=%d mode=%d\n",
                      port, mosi, miso, sclk, speed, mode);

        if (persist) {
            bool found = false;
            for (auto &e : config.spiConfigs) {
                if (e.port == port) {
                    e.mosiPin = mosi; e.misoPin = miso; e.sclkPin = sclk;
                    e.speed = speed; e.mode = mode; e.enabled = true;
                    found = true; break;
                }
            }
            if (!found && config.spiConfigs.size() < MAX_SPI_PORTS) {
                SpiEntry se;
                se.port = port; se.mosiPin = mosi; se.misoPin = miso;
                se.sclkPin = sclk; se.speed = speed; se.mode = mode;
                se.enabled = true;
                config.spiConfigs.push_back(se);
            }
            config.save();
        }

        JsonDocument resp;
        resp["type"]     = "spi_config";
        resp["deviceId"] = getDeviceId();
        resp["port"]     = port;
        resp["mosi"]     = mosi;
        resp["miso"]     = miso;
        resp["sclk"]     = sclk;
        resp["speed"]    = speed;
        resp["mode"]     = mode;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    // ---- write: 写入字节 (带 CS) ----
    } else if (action == "write") {
        int port  = doc["port"]  | 2;
        int mosi  = doc["mosi"]  | -1;
        int miso  = doc["miso"]  | -1;
        int sclk  = doc["sclk"]  | -1;
        int cs    = doc["cs"]    | -1;
        int speed = doc["speed"] | 1000000;
        int mode  = doc["mode"]  | 0;

        if (cs < 0) { Serial.println("[SPI] write: cs pin required"); return; }
        if (!isValidExternalPin(cs)) {
            Serial.printf("[SPI] REJECTED: cs=%d\n", cs); return;
        }

        SPIClass *s = ensureSPI(port, mosi, miso, sclk);
        if (!doc.containsKey("data")) { Serial.println("[SPI] write: data required"); return; }

        JsonArray arr = doc["data"].as<JsonArray>();
        uint8_t buf[512];
        int i = 0;
        for (JsonVariant v : arr) {
            if (i >= 512) break;
            buf[i++] = (uint8_t)v.as<int>();
        }

        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
        s->beginTransaction(SPISettings(speed, MSBFIRST, spiMode(mode)));
        digitalWrite(cs, LOW);
        s->transferBytes(buf, nullptr, i);
        digitalWrite(cs, HIGH);
        s->endTransaction();

        Serial.printf("[SPI] Write port=%d cs=%d %d bytes\n", port, cs, i);

    // ---- read: 读取字节 (带 CS) ----
    } else if (action == "read") {
        int port  = doc["port"]  | 2;
        int mosi  = doc["mosi"]  | -1;
        int miso  = doc["miso"]  | -1;
        int sclk  = doc["sclk"]  | -1;
        int cs    = doc["cs"]    | -1;
        int speed = doc["speed"] | 1000000;
        int mode  = doc["mode"]  | 0;
        int count = doc["count"] | 1;
        if (count > 512) count = 512;

        if (cs < 0) { Serial.println("[SPI] read: cs pin required"); return; }
        if (!isValidExternalPin(cs)) {
            Serial.printf("[SPI] REJECTED: cs=%d\n", cs); return;
        }

        SPIClass *s = ensureSPI(port, mosi, miso, sclk);
        uint8_t txBuf[512];
        uint8_t rxBuf[512];
        memset(txBuf, 0xFF, count);  // 发送 0xFF 作为 dummy

        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
        s->beginTransaction(SPISettings(speed, MSBFIRST, spiMode(mode)));
        digitalWrite(cs, LOW);
        s->transferBytes(txBuf, rxBuf, count);
        digitalWrite(cs, HIGH);
        s->endTransaction();

        JsonDocument resp;
        resp["type"]     = "spi_read";
        resp["deviceId"] = getDeviceId();
        resp["port"]     = port;
        resp["cs"]       = cs;
        resp["length"]   = count;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < count; i++) arr.add(rxBuf[i]);
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.printf("[SPI] Read port=%d cs=%d %d bytes\n", port, cs, count);

    // ---- transfer: 全双工读写 (带 CS) ----
    } else if (action == "transfer") {
        int port  = doc["port"]  | 2;
        int mosi  = doc["mosi"]  | -1;
        int miso  = doc["miso"]  | -1;
        int sclk  = doc["sclk"]  | -1;
        int cs    = doc["cs"]    | -1;
        int speed = doc["speed"] | 1000000;
        int mode  = doc["mode"]  | 0;

        if (cs < 0) { Serial.println("[SPI] transfer: cs pin required"); return; }
        if (!isValidExternalPin(cs)) {
            Serial.printf("[SPI] REJECTED: cs=%d\n", cs); return;
        }

        SPIClass *s = ensureSPI(port, mosi, miso, sclk);
        if (!doc.containsKey("data")) { Serial.println("[SPI] transfer: data required"); return; }

        JsonArray arr = doc["data"].as<JsonArray>();
        uint8_t txBuf[512], rxBuf[512];
        int i = 0;
        for (JsonVariant v : arr) {
            if (i >= 512) break;
            txBuf[i++] = (uint8_t)v.as<int>();
        }

        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
        s->beginTransaction(SPISettings(speed, MSBFIRST, spiMode(mode)));
        digitalWrite(cs, LOW);
        s->transferBytes(txBuf, rxBuf, i);
        digitalWrite(cs, HIGH);
        s->endTransaction();

        JsonDocument resp;
        resp["type"]     = "spi_transfer";
        resp["deviceId"] = getDeviceId();
        resp["port"]     = port;
        resp["cs"]       = cs;
        resp["length"]   = i;
        JsonArray sentArr = resp["sent"].to<JsonArray>();
        JsonArray recvArr = resp["received"].to<JsonArray>();
        for (int j = 0; j < i; j++) {
            sentArr.add(txBuf[j]);
            recvArr.add(rxBuf[j]);
        }
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.printf("[SPI] Transfer port=%d cs=%d %d bytes\n", port, cs, i);

    // ---- close: 释放 SPI 总线 ----
    } else if (action == "close") {
        if (spi) {
            spi->end();
            delete spi;
            spi = nullptr;
            curPort = -1;
            Serial.println("[SPI] Bus released");
        }
        for (auto it = config.spiConfigs.begin(); it != config.spiConfigs.end(); ++it) {
            config.spiConfigs.erase(it); config.save(); break;
        }

    // ---- list: 列出配置 ----
    } else if (action == "list") {
        JsonDocument resp;
        resp["type"]     = "spi_list";
        resp["deviceId"] = getDeviceId();
        JsonArray arr = resp["buses"].to<JsonArray>();
        for (auto &e : config.spiConfigs) {
            JsonObject o = arr.add<JsonObject>();
            o["port"]    = e.port;
            o["mosi"]    = e.mosiPin;
            o["miso"]    = e.misoPin;
            o["sclk"]    = e.sclkPin;
            o["speed"]   = e.speed;
            o["mode"]    = e.mode;
            o["enabled"] = e.enabled;
        }
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.println("[SPI] List published");

    } else {
        Serial.printf("[SPI] Unknown action: %s\n", action.c_str());
    }
}

} // namespace SPIBus
