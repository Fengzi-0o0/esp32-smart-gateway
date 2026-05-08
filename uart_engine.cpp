#include "uart_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include "pin_caps.h"
#include "script_engine.h"


static bool uart1Active = false;
static bool uart2Active = false;
static bool uart1Listening = false;
static bool uart2Listening = false;

// ========== 串口配置字构�?==========
// parity: 0=none, 1=even, 2=odd
// dataBits: 5,6,7,8   stopBits: 1,2

namespace UART {

uint32_t buildConfig(int dataBits, int parity, int stopBits) {
    if (dataBits == 8) {
        if (parity == 0) return (stopBits == 2) ? SERIAL_8N2 : SERIAL_8N1;
        if (parity == 1) return (stopBits == 2) ? SERIAL_8E2 : SERIAL_8E1;
        return (stopBits == 2) ? SERIAL_8O2 : SERIAL_8O1;
    } else if (dataBits == 7) {
        if (parity == 0) return (stopBits == 2) ? SERIAL_7N2 : SERIAL_7N1;
        if (parity == 1) return (stopBits == 2) ? SERIAL_7E2 : SERIAL_7E1;
        return (stopBits == 2) ? SERIAL_7O2 : SERIAL_7O1;
    } else if (dataBits == 6) {
        if (parity == 0) return (stopBits == 2) ? SERIAL_6N2 : SERIAL_6N1;
        if (parity == 1) return (stopBits == 2) ? SERIAL_6E2 : SERIAL_6E1;
        return (stopBits == 2) ? SERIAL_6O2 : SERIAL_6O1;
    } else { // 5
        if (parity == 0) return (stopBits == 2) ? SERIAL_5N2 : SERIAL_5N1;
        if (parity == 1) return (stopBits == 2) ? SERIAL_5E2 : SERIAL_5E1;
        return (stopBits == 2) ? SERIAL_5O2 : SERIAL_5O1;
    }
}

// ========== 工具：Hex 字符串转字节数组 ==========
static int parseHex(const String &hex, uint8_t *buf, int maxLen) {
    String clean = "";
    for (int i = 0; i < (int)hex.length(); i++) {
        char c = hex.charAt(i);
        if (c != ' ' && c != ':' && c != '-') clean += c;
    }
    int len = clean.length() / 2;
    if (len > maxLen) len = maxLen;
    for (int i = 0; i < len; i++) {
        String byteStr = clean.substring(i * 2, i * 2 + 2);
        buf[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    return len;
}

// ========== 工具：发布串口数据到 MQTT ==========
static void publishUartData(int port, const uint8_t *data, int len) {
    JsonDocument doc;
    doc["type"]     = "uart_data";
    doc["deviceId"] = getDeviceId();
    doc["port"]     = port;
    doc["length"]   = len;

    JsonArray arr = doc["data"].to<JsonArray>();
    String text = "";
    for (int i = 0; i < len; i++) {
        arr.add(data[i]);
        if (data[i] >= 32 && data[i] < 127) text += (char)data[i];
        else text += ".";
    }
    doc["text"] = text;

    String p;
    serializeJson(doc, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
        MqttClient::publish(config.pubTopics[0].topic, p);
}

// ========== 初始化：恢复持久化配�?==========

void init() {
    uart1Active = false;
    uart2Active = false;
    uart1Listening = false;
    uart2Listening = false;

    // �?NVS 恢复持久�?UART 配置
    for (auto &ue : config.uartConfigs) {
        if (!ue.enabled) continue;
        uint32_t cfg = buildConfig(ue.dataBits, ue.parity, ue.stopBits);
        if (ue.port == 1) {
            if (uart1Active) Serial1.end();
            Serial1.begin(ue.baud, cfg, ue.rxPin, ue.txPin);
            uart1Active = true;
            uart1Listening = ue.listening;
            Serial.printf("[UART] Restored UART1 TX=%d RX=%d baud=%d %d%c%d %s\n",
                          ue.txPin, ue.rxPin, ue.baud, ue.dataBits,
                          ue.parity == 0 ? 'N' : (ue.parity == 1 ? 'E' : 'O'),
                          ue.stopBits,
                          ue.listening ? "(listening)" : "");
        } else if (ue.port == 2) {
            if (uart2Active) Serial2.end();
            Serial2.begin(ue.baud, cfg, ue.rxPin, ue.txPin);
            uart2Active = true;
            uart2Listening = ue.listening;
            Serial.printf("[UART] Restored UART2 TX=%d RX=%d baud=%d %d%c%d %s\n",
                          ue.txPin, ue.rxPin, ue.baud, ue.dataBits,
                          ue.parity == 0 ? 'N' : (ue.parity == 1 ? 'E' : 'O'),
                          ue.stopBits,
                          ue.listening ? "(listening)" : "");
        }
    }
    Serial.printf("[UART] Engine initialized (%d configs)\n", config.uartConfigs.size());
}

// ========== 主循环：监听模式数据接收 ==========

void loop() {
    // UART1 监听
    if (uart1Active && uart1Listening && Serial1.available()) {
        uint8_t buf[256];
        int idx = 0;
        while (Serial1.available() && idx < 256) {
            buf[idx++] = Serial1.read();
        }
        if (idx > 0) {
            publishUartData(1, buf, idx);
            Serial.printf("[UART1] Received %d bytes\n", idx);
        }
    }

    // UART2 监听
    if (uart2Active && uart2Listening && Serial2.available()) {
        uint8_t buf[256];
        int idx = 0;
        while (Serial2.available() && idx < 256) {
            buf[idx++] = Serial2.read();
        }
        if (idx > 0) {
            publishUartData(2, buf, idx);
            Serial.printf("[UART2] Received %d bytes\n", idx);
        }
    }
}

// ========== 命令处理 ==========

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();
    int port = doc["port"] | 1;

    // ---- config: 配置并启动串�?----
    if (action == "config") {
        if (port != 1 && port != 2) {
            Serial.println("[UART] port must be 1 or 2");
            return;
        }
        int txPin    = doc["tx"]       | -1;
        int rxPin    = doc["rx"]       | -1;
        int baud     = doc["baud"]     | 9600;
        int dataBits = doc["dataBits"] | 8;
        int stopBits = doc["stopBits"] | 1;
        int parity   = doc["parity"]   | 0;  // 0=none,1=even,2=odd
        bool listen  = doc["listen"]   | false;
        bool persist = doc["persistent"] | false;

        // 引脚验证
        if (txPin >= 0 && !isValidExternalPin(txPin)) {
            Serial.printf("[UART] REJECTED: tx=%d not in external pinout\n", txPin);
            return;
        }
        if (rxPin >= 0 && !isValidExternalPin(rxPin)) {
            Serial.printf("[UART] REJECTED: rx=%d not in external pinout\n", rxPin);
            return;
        }

        uint32_t cfg = buildConfig(dataBits, parity, stopBits);

        if (port == 1) {
            if (uart1Active) Serial1.end();
            Serial1.begin(baud, cfg, rxPin, txPin);
            uart1Active = true;
            uart1Listening = listen;
        } else {
            if (uart2Active) Serial2.end();
            Serial2.begin(baud, cfg, rxPin, txPin);
            uart2Active = true;
            uart2Listening = listen;
        }

        Serial.printf("[UART] Configured UART%d TX=%d RX=%d baud=%d %d%c%d %s\n",
                      port, txPin, rxPin, baud, dataBits,
                      parity == 0 ? 'N' : (parity == 1 ? 'E' : 'O'),
                      stopBits,
                      listen ? "(listening)" : "");

        // 持久�?
        if (persist) {
            bool found = false;
            for (auto &e : config.uartConfigs) {
                if (e.port == port) {
                    e.txPin = txPin; e.rxPin = rxPin; e.baud = baud;
                    e.dataBits = dataBits; e.stopBits = stopBits;
                    e.parity = parity; e.enabled = true; e.listening = listen;
                    found = true; break;
                }
            }
            if (!found && config.uartConfigs.size() < MAX_UART_PORTS) {
                UartEntry ue;
                ue.port = port; ue.txPin = txPin; ue.rxPin = rxPin;
                ue.baud = baud; ue.dataBits = dataBits; ue.stopBits = stopBits;
                ue.parity = parity; ue.enabled = true; ue.listening = listen;
                config.uartConfigs.push_back(ue);
            }
            config.markDirty();
        }

        JsonDocument resp;
        resp["type"]     = "uart_config";
        resp["deviceId"] = getDeviceId();
        resp["port"]     = port;
        resp["tx"]       = txPin;
        resp["rx"]       = rxPin;
        resp["baud"]     = baud;
        resp["dataBits"] = dataBits;
        resp["stopBits"] = stopBits;
        resp["parity"]   = parity;
        resp["listening"] = listen;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    // ---- write: 发送字符串 ----
    } else if (action == "write") {
        String data = doc["data"].as<String>();
        if (data.length() == 0) { Serial.println("[UART] write: data required"); return; }
        if (port == 1 && uart1Active) {
            Serial1.print(data);
            Serial.printf("[UART1] TX: %s\n", data.c_str());
        } else if (port == 2 && uart2Active) {
            Serial2.print(data);
            Serial.printf("[UART2] TX: %s\n", data.c_str());
        } else {
            Serial.printf("[UART] Port %d not active\n", port);
        }

    // ---- write_bytes: 发送字节数�?----
    } else if (action == "write_bytes") {
        if (!doc.containsKey("data")) { Serial.println("[UART] write_bytes: data array required"); return; }
        JsonArray arr = doc["data"].as<JsonArray>();
        uint8_t buf[256];
        int i = 0;
        for (JsonVariant v : arr) {
            if (i >= 256) break;
            buf[i++] = (uint8_t)v.as<int>();
        }
        if (port == 1 && uart1Active) {
            Serial1.write(buf, i);
            Serial.printf("[UART1] TX %d bytes\n", i);
        } else if (port == 2 && uart2Active) {
            Serial2.write(buf, i);
            Serial.printf("[UART2] TX %d bytes\n", i);
        } else {
            Serial.printf("[UART] Port %d not active\n", port);
        }

    // ---- write_hex: 发�?Hex 字符�?----
    } else if (action == "write_hex") {
        String hex = doc["hex"].as<String>();
        if (hex.length() == 0) { Serial.println("[UART] write_hex: hex required"); return; }
        uint8_t buf[256];
        int len = parseHex(hex, buf, 256);
        if (port == 1 && uart1Active) {
            Serial1.write(buf, len);
            Serial.printf("[UART1] TX %d bytes (hex)\n", len);
        } else if (port == 2 && uart2Active) {
            Serial2.write(buf, len);
            Serial.printf("[UART2] TX %d bytes (hex)\n", len);
        } else {
            Serial.printf("[UART] Port %d not active\n", port);
        }

    // ---- read: 读取当前缓冲区数�?----
    } else if (action == "read") {
        HardwareSerial *serial = (port == 1) ? &Serial1 : &Serial2;
        bool active = (port == 1) ? uart1Active : uart2Active;
        if (!active) { Serial.printf("[UART] Port %d not active\n", port); return; }

        uint8_t buf[256];
        int idx = 0;
        while (serial->available() && idx < 256) {
            buf[idx++] = serial->read();
        }
        if (idx > 0) {
            publishUartData(port, buf, idx);
            Serial.printf("[UART%d] Read %d bytes\n", port, idx);
        } else {
            Serial.printf("[UART%d] No data available\n", port);
            JsonDocument resp;
            resp["type"]     = "uart_data";
            resp["deviceId"] = getDeviceId();
            resp["port"]     = port;
            resp["length"]   = 0;
            resp["text"]     = "";
                if (doc.containsKey("result_var")) {
        String varName = doc["result_var"].as<String>();
        if (varName.length() > 0)
            ScriptEngine::setVar(varName,(float)buf[0], false);
    }

            String p;
            serializeJson(resp, p);
            if (MqttClient::isConnected() && config.pubTopics.size() > 0)
                MqttClient::publish(config.pubTopics[0].topic, p);
        }

    // ---- listen_start: 开启监�?----
    } else if (action == "listen_start") {
        if (port == 1) { uart1Listening = true; Serial.println("[UART1] Listening started"); }
        else if (port == 2) { uart2Listening = true; Serial.println("[UART2] Listening started"); }
        // 更新 NVS
        for (auto &e : config.uartConfigs) {
            if (e.port == port) { e.listening = true; config.markDirty(); break; }
        }

    // ---- listen_stop: 停止监听 ----
    } else if (action == "listen_stop") {
        if (port == 1) { uart1Listening = false; Serial.println("[UART1] Listening stopped"); }
        else if (port == 2) { uart2Listening = false; Serial.println("[UART2] Listening stopped"); }
        for (auto &e : config.uartConfigs) {
            if (e.port == port) { e.listening = false; config.markDirty(); break; }
        }

    // ---- close: 关闭串口 ----
    } else if (action == "close") {
        if (port == 1 && uart1Active) {
            Serial1.end(); uart1Active = false; uart1Listening = false;
            Serial.println("[UART1] Closed");
        } else if (port == 2 && uart2Active) {
            Serial2.end(); uart2Active = false; uart2Listening = false;
            Serial.println("[UART2] Closed");
        }
        for (auto it = config.uartConfigs.begin(); it != config.uartConfigs.end(); ++it) {
            if (it->port == port) { config.uartConfigs.erase(it); config.markDirty(); break; }
        }

    // ---- list: 列出配置 ----
    } else if (action == "list") {
        JsonDocument resp;
        resp["type"]     = "uart_list";
        resp["deviceId"] = getDeviceId();
        JsonArray arr = resp["ports"].to<JsonArray>();
        for (auto &e : config.uartConfigs) {
            JsonObject o = arr.add<JsonObject>();
            o["port"]      = e.port;
            o["tx"]        = e.txPin;
            o["rx"]        = e.rxPin;
            o["baud"]      = e.baud;
            o["dataBits"]  = e.dataBits;
            o["stopBits"]  = e.stopBits;
            o["parity"]    = e.parity;
            o["enabled"]   = e.enabled;
            o["listening"] = e.listening;
            o["active"]    = (e.port == 1) ? uart1Active : uart2Active;
        }
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.println("[UART] List published");

    } else {
        Serial.printf("[UART] Unknown action: %s\n", action.c_str());
    }
}

} // namespace UART
