#ifndef DATA_ENGINE_H
#define DATA_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace DataEngine {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);

    // 工具函数（其他引擎也可调用）
    int  hexToBytes(const String &hex, uint8_t *buf, int maxLen);
    String bytesToHex(const uint8_t *buf, int len);
    uint16_t crc16(const uint8_t *buf, int len);
}

#endif
