#include "data_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include "script_engine.h"
#include <math.h>

// ================================================================
//  内部工具函数
// ================================================================

// Hex 字符串 → 字节数组，支持 "FF 01 A2" / "FF:01:A2" / "FF01A2"
int DataEngine::hexToBytes(const String &hex, uint8_t *buf, int maxLen) {
    String clean = "";
    for (int i = 0; i < (int)hex.length(); i++) {
        char c = hex.charAt(i);
        if (c != ' ' && c != ':' && c != '-' && c != '\t')
            clean += c;
    }
    int len = clean.length() / 2;
    if (len > maxLen) len = maxLen;
    for (int i = 0; i < len; i++) {
        String byteStr = clean.substring(i * 2, i * 2 + 2);
        buf[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    return len;
}

// 字节数组 → Hex 字符串（大写，无分隔符）
String DataEngine::bytesToHex(const uint8_t *buf, int len) {
    String result = "";
    result.reserve(len * 2);
    for (int i = 0; i < len; i++) {
        char tmp[4];
        snprintf(tmp, sizeof(tmp), "%02X", buf[i]);
        result += tmp;
    }
    return result;
}

// CRC-16/Modbus
uint16_t DataEngine::crc16(const uint8_t *buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// 辅助：从 JSON 数组读取字节数组
static int readByteArray(JsonDocument &doc, const char *key, uint8_t *buf, int maxLen) {
    if (!doc.containsKey(key)) return 0;
    JsonArray arr = doc[key].as<JsonArray>();
    int i = 0;
    for (JsonVariant v : arr) {
        if (i >= maxLen) break;
        buf[i++] = (uint8_t)v.as<int>();
    }
    return i;
}

// 辅助：发布结果到 MQTT
static void publishResult(JsonDocument &resp) {
    String p;
    serializeJson(resp, p);
    if (MqttClient::isConnected() && config.pubTopics.size() > 0)
        MqttClient::publish(config.pubTopics[0].topic.c_str(), p.c_str());
}

// 辅助：存入脚本变量（如果指定了 result_var）
static void storeVar(JsonDocument &doc, float value) {
    if (doc.containsKey("result_var")) {
        String varName = doc["result_var"].as<String>();
        if (varName.length() > 0)
            ScriptEngine::setVar(varName, value, false);
    }
}

static void storeVar(JsonDocument &doc, const String &value) {
    if (doc.containsKey("result_var")) {
        String varName = doc["result_var"].as<String>();
        if (varName.length() > 0)
            ScriptEngine::setVar(varName, value.toFloat(), false);
    }
}

// ================================================================
//  引擎生命周期
// ================================================================

namespace DataEngine {

void init() {
    Serial.println("[DATA] Engine initialized");
}

void loop() {
    // 无状态引擎，无需循环处理
}

// ================================================================
//  命令分发
// ================================================================

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();

    // ─────────────────────────────────────
    //  hex_to_bytes: "FF01A2" → [255,1,162]
    // ─────────────────────────────────────
    if (action == "hex_to_bytes") {
        String hex = doc["hex"].as<String>();
        if (hex.length() == 0) { Serial.println("[DATA] hex_to_bytes: hex required"); return; }

        uint8_t buf[256];
        int len = hexToBytes(hex, buf, 256);

        JsonDocument resp;
        resp["type"]     = "data_hex_to_bytes";
        resp["deviceId"] = getDeviceId();
        resp["input"]    = hex;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < len; i++) arr.add(buf[i]);
        resp["length"] = len;
        publishResult(resp);

        Serial.printf("[DATA] hex_to_bytes: %s → %d bytes\n", hex.c_str(), len);

    // ─────────────────────────────────────
    //  bytes_to_hex: [255,1,162] → "FF01A2"
    // ─────────────────────────────────────
    } else if (action == "bytes_to_hex") {
        uint8_t buf[256];
        int len = readByteArray(doc, "data", buf, 256);
        if (len == 0) { Serial.println("[DATA] bytes_to_hex: data required"); return; }

        String hex = bytesToHex(buf, len);

        JsonDocument resp;
        resp["type"]     = "data_bytes_to_hex";
        resp["deviceId"] = getDeviceId();
        resp["hex"]      = hex;
        resp["length"]   = len;
        publishResult(resp);

        storeVar(doc, hex);
        Serial.printf("[DATA] bytes_to_hex: %d bytes → %s\n", len, hex.c_str());

    // ─────────────────────────────────────
    //  bytes_to_int: [0,45] → 45
    //  endian: "big" (默认) / "little"
    //  signed: true / false (默认 false)
    //  length: 可选，默认取 data 全部长度
    // ─────────────────────────────────────
    } else if (action == "bytes_to_int") {
        uint8_t buf[8];
        int len = readByteArray(doc, "data", buf, 8);
        if (len == 0) { Serial.println("[DATA] bytes_to_int: data required"); return; }

        String endian = doc["endian"] | String("big");
        bool isSigned = doc["signed"] | false;

        uint64_t val = 0;
        if (endian == "big") {
            for (int i = 0; i < len; i++)
                val = (val << 8) | buf[i];
        } else {
            for (int i = len - 1; i >= 0; i--)
                val = (val << 8) | buf[i];
        }

        JsonDocument resp;
        resp["type"]     = "data_bytes_to_int";
        resp["deviceId"] = getDeviceId();
        resp["endian"]   = endian;
        resp["signed"]   = isSigned;
        resp["length"]   = len;

        if (isSigned) {
            int64_t sVal = (int64_t)val;
            // 符号扩展
            if (len <= 1 && (buf[endian == "big" ? 0 : len-1] & 0x80))
                sVal -= (1LL << (len * 8));
            resp["value"] = sVal;
            storeVar(doc, (float)sVal);
        } else {
            resp["value"] = (uint64_t)val;
            storeVar(doc, (float)val);
        }

        publishResult(resp);
        Serial.printf("[DATA] bytes_to_int: %d bytes (%s) → %llu\n",
                      len, endian.c_str(), val);

    // ─────────────────────────────────────
    //  int_to_bytes: 45 → [0,45]
    //  endian: "big" (默认) / "little"
    //  length: 字节数 (默认 2)
    // ─────────────────────────────────────
    } else if (action == "int_to_bytes") {
        if (!doc.containsKey("value")) {
            Serial.println("[DATA] int_to_bytes: value required"); return;
        }
        uint64_t val = doc["value"].as<uint64_t>();
        String endian = doc["endian"] | String("big");
        int length = doc["length"] | 2;
        if (length < 1) length = 1;
        if (length > 8) length = 8;

        uint8_t buf[8] = {0};
        for (int i = 0; i < length; i++) {
            if (endian == "big")
                buf[length - 1 - i] = (val >> (i * 8)) & 0xFF;
            else
                buf[i] = (val >> (i * 8)) & 0xFF;
        }

        JsonDocument resp;
        resp["type"]     = "data_int_to_bytes";
        resp["deviceId"] = getDeviceId();
        resp["value"]    = val;
        resp["endian"]   = endian;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < length; i++) arr.add(buf[i]);
        resp["length"] = length;
        publishResult(resp);

        Serial.printf("[DATA] int_to_bytes: %llu → %d bytes (%s)\n",
                      val, length, endian.c_str());

    // ─────────────────────────────────────
    //  bytes_to_float: [64,72,0,0] → 3.125
    //  IEEE754 单精度 4 字节
    //  endian: "big" (默认) / "little"
    // ─────────────────────────────────────
    } else if (action == "bytes_to_float") {
        uint8_t buf[4];
        int len = readByteArray(doc, "data", buf, 4);
        if (len != 4) {
            Serial.println("[DATA] bytes_to_float: need exactly 4 bytes"); return;
        }

        String endian = doc["endian"] | String("big");
        uint32_t raw = 0;
        if (endian == "big") {
            raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
                  ((uint32_t)buf[2] << 8)  | buf[3];
        } else {
            raw = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) |
                  ((uint32_t)buf[1] << 8)  | buf[0];
        }

        float fVal;
        memcpy(&fVal, &raw, sizeof(float));

        JsonDocument resp;
        resp["type"]     = "data_bytes_to_float";
        resp["deviceId"] = getDeviceId();
        resp["value"]    = fVal;
        resp["endian"]   = endian;
        resp["raw"]      = raw;
        publishResult(resp);

        storeVar(doc, fVal);
        Serial.printf("[DATA] bytes_to_float: [0x%02X,0x%02X,0x%02X,0x%02X] → %.6f\n",
                      buf[0], buf[1], buf[2], buf[3], fVal);

    // ─────────────────────────────────────
    //  float_to_bytes: 3.125 → [64,72,0,0]
    //  endian: "big" (默认) / "little"
    // ─────────────────────────────────────
    } else if (action == "float_to_bytes") {
        if (!doc.containsKey("value")) {
            Serial.println("[DATA] float_to_bytes: value required"); return;
        }
        float fVal = doc["value"].as<float>();
        String endian = doc["endian"] | String("big");

        uint32_t raw;
        memcpy(&raw, &fVal, sizeof(float));

        uint8_t buf[4];
        if (endian == "big") {
            buf[0] = (raw >> 24) & 0xFF;
            buf[1] = (raw >> 16) & 0xFF;
            buf[2] = (raw >> 8)  & 0xFF;
            buf[3] = raw & 0xFF;
        } else {
            buf[3] = (raw >> 24) & 0xFF;
            buf[2] = (raw >> 16) & 0xFF;
            buf[1] = (raw >> 8)  & 0xFF;
            buf[0] = raw & 0xFF;
        }

        JsonDocument resp;
        resp["type"]     = "data_float_to_bytes";
        resp["deviceId"] = getDeviceId();
        resp["value"]    = fVal;
        resp["endian"]   = endian;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < 4; i++) arr.add(buf[i]);
        publishResult(resp);

        Serial.printf("[DATA] float_to_bytes: %.6f → [0x%02X,0x%02X,0x%02X,0x%02X]\n",
                      fVal, buf[0], buf[1], buf[2], buf[3]);

    // ─────────────────────────────────────
    //  bytes_to_ascii: [72,101,108] → "Hel"
    // ─────────────────────────────────────
    } else if (action == "bytes_to_ascii") {
        uint8_t buf[256];
        int len = readByteArray(doc, "data", buf, 256);
        if (len == 0) { Serial.println("[DATA] bytes_to_ascii: data required"); return; }

        String text = "";
        for (int i = 0; i < len; i++) {
            if (buf[i] >= 32 && buf[i] < 127)
                text += (char)buf[i];
            else
                text += ".";
        }

        JsonDocument resp;
        resp["type"]     = "data_bytes_to_ascii";
        resp["deviceId"] = getDeviceId();
        resp["text"]     = text;
        resp["length"]   = len;
        publishResult(resp);

        Serial.printf("[DATA] bytes_to_ascii: %d bytes → \"%s\"\n", len, text.c_str());

    // ─────────────────────────────────────
    //  ascii_to_bytes: "AT+RST" → [65,84,43,82,83,84]
    // ─────────────────────────────────────
    } else if (action == "ascii_to_bytes") {
        String text = doc["text"].as<String>();
        if (text.length() == 0) { Serial.println("[DATA] ascii_to_bytes: text required"); return; }

        JsonDocument resp;
        resp["type"]     = "data_ascii_to_bytes";
        resp["deviceId"] = getDeviceId();
        resp["text"]     = text;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < (int)text.length(); i++)
            arr.add((uint8_t)text.charAt(i));
        resp["length"] = text.length();
        publishResult(resp);

        Serial.printf("[DATA] ascii_to_bytes: \"%s\" → %d bytes\n",
                      text.c_str(), text.length());

    // ─────────────────────────────────────
    //  to_binary: 202 → "11001010"
    //  可选 bits: 指定位数（默认自动）
    // ─────────────────────────────────────
    } else if (action == "to_binary") {
        if (!doc.containsKey("value")) {
            Serial.println("[DATA] to_binary: value required"); return;
        }
        uint32_t val = doc["value"].as<uint32_t>();
        int bits = doc["bits"] | 0;

        if (bits <= 0) {
            // 自动计算最少需要的位数
            bits = 1;
            uint32_t tmp = val;
            while (tmp >>= 1) bits++;
        }
        if (bits > 32) bits = 32;

        String binary = "";
        for (int i = bits - 1; i >= 0; i--)
            binary += (val & (1u << i)) ? '1' : '0';

        JsonDocument resp;
        resp["type"]     = "data_to_binary";
        resp["deviceId"] = getDeviceId();
        resp["value"]    = val;
        resp["binary"]   = binary;
        resp["bits"]     = bits;
        publishResult(resp);

        storeVar(doc, (float)val);
        Serial.printf("[DATA] to_binary: %u → %s (%d bits)\n",
                      val, binary.c_str(), bits);

    // ─────────────────────────────────────
    //  from_binary: "11001010" → 202
    // ─────────────────────────────────────
    } else if (action == "from_binary") {
        String binary = doc["binary"].as<String>();
        if (binary.length() == 0) {
            Serial.println("[DATA] from_binary: binary required"); return;
        }

        uint32_t val = 0;
        for (int i = 0; i < (int)binary.length(); i++) {
            char c = binary.charAt(i);
            if (c == '1') val = (val << 1) | 1;
            else val <<= 1;
        }

        JsonDocument resp;
        resp["type"]     = "data_from_binary";
        resp["deviceId"] = getDeviceId();
        resp["binary"]   = binary;
        resp["value"]    = val;
        resp["bits"]     = binary.length();
        publishResult(resp);

        storeVar(doc, (float)val);
        Serial.printf("[DATA] from_binary: %s → %u\n", binary.c_str(), val);

    // ─────────────────────────────────────
    //  crc16: CRC-16/Modbus 校验
    //  data: 字节数组
    //  可选 append: true → 在 data 后追加 CRC 低字节+高字节
    // ─────────────────────────────────────
    } else if (action == "crc16") {
        uint8_t buf[256];
        int len = readByteArray(doc, "data", buf, 256);
        if (len == 0) { Serial.println("[DATA] crc16: data required"); return; }

        bool append = doc["append"] | false;
        uint16_t crc = DataEngine::crc16(buf, len);

        JsonDocument resp;
        resp["type"]     = "data_crc16";
        resp["deviceId"] = getDeviceId();
        resp["crc"]      = crc;
        resp["crcHigh"]  = (crc >> 8) & 0xFF;
        resp["crcLow"]   = crc & 0xFF;

        if (append) {
            uint8_t fullBuf[258];
            memcpy(fullBuf, buf, len);
            fullBuf[len]     = crc & 0xFF;        // Modbus: 低字节在前
            fullBuf[len + 1] = (crc >> 8) & 0xFF;
            int fullLen = len + 2;

            JsonArray arr = resp["data"].to<JsonArray>();
            for (int i = 0; i < fullLen; i++) arr.add(fullBuf[i]);
            resp["length"] = fullLen;

            // 同时更新 Hex
            resp["hex"] = DataEngine::bytesToHex(fullBuf, fullLen);
        }

        publishResult(resp);
        storeVar(doc, (float)crc);
        Serial.printf("[DATA] crc16: %d bytes → 0x%04X\n", len, crc);

    // ─────────────────────────────────────
    //  concat: 拼接多个字节数组
    //  arrays: [[1,2],[3,4],[5]] → [1,2,3,4,5]
    // ─────────────────────────────────────
    } else if (action == "concat") {
        if (!doc.containsKey("arrays")) {
            Serial.println("[DATA] concat: arrays required"); return;
        }

        uint8_t buf[512];
        int totalLen = 0;

        JsonArray outerArr = doc["arrays"].as<JsonArray>();
        for (JsonVariant innerVar : outerArr) {
            JsonArray innerArr = innerVar.as<JsonArray>();
            for (JsonVariant v : innerArr) {
                if (totalLen >= 512) break;
                buf[totalLen++] = (uint8_t)v.as<int>();
            }
        }

        JsonDocument resp;
        resp["type"]     = "data_concat";
        resp["deviceId"] = getDeviceId();
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < totalLen; i++) arr.add(buf[i]);
        resp["length"] = totalLen;
        resp["hex"]    = bytesToHex(buf, totalLen);
        publishResult(resp);

        Serial.printf("[DATA] concat: %d arrays → %d bytes\n",
                      outerArr.size(), totalLen);

    // ─────────────────────────────────────
    //  slice: 截取子数组
    //  data: [1,2,3,4,5]
    //  offset: 起始位置 (从0开始)
    //  length: 截取长度
    // ─────────────────────────────────────
    } else if (action == "slice") {
        uint8_t buf[256];
        int srcLen = readByteArray(doc, "data", buf, 256);
        if (srcLen == 0) { Serial.println("[DATA] slice: data required"); return; }

        int offset = doc["offset"] | 0;
        int length = doc["length"] | 1;
        if (offset < 0) offset = 0;
        if (offset >= srcLen) {
            Serial.println("[DATA] slice: offset out of range"); return;
        }
        if (offset + length > srcLen) length = srcLen - offset;

        JsonDocument resp;
        resp["type"]     = "data_slice";
        resp["deviceId"] = getDeviceId();
        resp["offset"]   = offset;
        JsonArray arr = resp["data"].to<JsonArray>();
        for (int i = 0; i < length; i++) arr.add(buf[offset + i]);
        resp["length"] = length;
        resp["hex"]    = bytesToHex(buf + offset, length);
        publishResult(resp);

        Serial.printf("[DATA] slice: offset=%d length=%d\n", offset, length);

    // ─────────────────────────────────────
    //  extract: 从数据中提取多段字段
    //  一条指令完成协议解析
    //  data: 原始字节数组
    //  fields: [{"name":"type","offset":0,"length":1},
    //           {"name":"value","offset":1,"length":2,"endian":"big"}]
    // ─────────────────────────────────────
    } else if (action == "extract") {
        uint8_t buf[256];
        int srcLen = readByteArray(doc, "data", buf, 256);
        if (srcLen == 0) { Serial.println("[DATA] extract: data required"); return; }

        if (!doc.containsKey("fields")) {
            Serial.println("[DATA] extract: fields required"); return;
        }

        JsonDocument resp;
        resp["type"]     = "data_extract";
        resp["deviceId"] = getDeviceId();
        JsonObject fields = resp["fields"].to<JsonObject>();

        for (JsonObject field : doc["fields"].as<JsonArray>()) {
            String name   = field["name"] | String("");
            int offset    = field["offset"] | 0;
            int length    = field["length"] | 1;
            String endian = field["endian"] | String("big");
            String as     = field["as"] | String("int");  // "int","hex","ascii","bytes"

            if (name.length() == 0) continue;
            if (offset < 0 || offset + length > srcLen) {
                fields[name] = "OUT_OF_RANGE";
                continue;
            }

            uint8_t *ptr = buf + offset;

            if (as == "hex") {
                fields[name] = bytesToHex(ptr, length);
            } else if (as == "ascii") {
                String text = "";
                for (int i = 0; i < length; i++) {
                    if (ptr[i] >= 32 && ptr[i] < 127) text += (char)ptr[i];
                    else text += ".";
                }
                fields[name] = text;
            } else if (as == "bytes") {
                JsonArray arr = fields[name].to<JsonArray>();
                for (int i = 0; i < length; i++) arr.add(ptr[i]);
            } else if (as == "float" && length == 4) {
                uint32_t raw = 0;
                if (endian == "big")
                    raw = ((uint32_t)ptr[0]<<24)|((uint32_t)ptr[1]<<16)|((uint32_t)ptr[2]<<8)|ptr[3];
                else
                    raw = ((uint32_t)ptr[3]<<24)|((uint32_t)ptr[2]<<16)|((uint32_t)ptr[1]<<8)|ptr[0];
                float fVal;
                memcpy(&fVal, &raw, sizeof(float));
                fields[name] = fVal;
            } else {
                // 默认: int
                uint64_t val = 0;
                if (endian == "big") {
                    for (int i = 0; i < length; i++)
                        val = (val << 8) | ptr[i];
                } else {
                    for (int i = length - 1; i >= 0; i--)
                        val = (val << 8) | ptr[i];
                }
                fields[name] = val;
            }
        }

        publishResult(resp);
        Serial.printf("[DATA] extract: %d fields from %d bytes\n",
                      doc["fields"].as<JsonArray>().size(), srcLen);

    // ─────────────────────────────────────
    //  compare: 比较两个字节数组
    // ─────────────────────────────────────
    } else if (action == "compare") {
        uint8_t bufA[256], bufB[256];
        int lenA = readByteArray(doc, "data_a", bufA, 256);
        int lenB = readByteArray(doc, "data_b", bufB, 256);

        bool equal = (lenA == lenB) && (memcmp(bufA, bufB, lenA) == 0);

        JsonDocument resp;
        resp["type"]     = "data_compare";
        resp["deviceId"] = getDeviceId();
        resp["equal"]    = equal;
        resp["lenA"]     = lenA;
        resp["lenB"]     = lenB;
        publishResult(resp);

        Serial.printf("[DATA] compare: %d vs %d bytes → %s\n",
                      lenA, lenB, equal ? "EQUAL" : "DIFFERENT");

    // ─────────────────────────────────────
    //  bit: 位操作
    //  value: 原始整数
    //  op: "get" / "set" / "clear" / "toggle"
    //  bit: 位号 (0-31)
    // ─────────────────────────────────────
    } else if (action == "bit") {
        if (!doc.containsKey("value") || !doc.containsKey("bit")) {
            Serial.println("[DATA] bit: value and bit required"); return;
        }
        uint32_t val = doc["value"].as<uint32_t>();
        int bitNum   = doc["bit"].as<int>();
        String op    = doc["op"] | String("get");

        if (bitNum < 0 || bitNum > 31) {
            Serial.println("[DATA] bit: bit must be 0-31"); return;
        }

        uint32_t mask = 1u << bitNum;
        uint32_t result = val;

        if (op == "get") {
            result = (val & mask) ? 1 : 0;
        } else if (op == "set") {
            result = val | mask;
        } else if (op == "clear") {
            result = val & ~mask;
        } else if (op == "toggle") {
            result = val ^ mask;
        }

        JsonDocument resp;
        resp["type"]     = "data_bit";
        resp["deviceId"] = getDeviceId();
        resp["op"]       = op;
        resp["bit"]      = bitNum;
        resp["input"]    = val;
        resp["output"]   = result;
        publishResult(resp);

        storeVar(doc, (float)result);
        Serial.printf("[DATA] bit %s: val=%u bit=%d → %u\n",
                      op.c_str(), val, bitNum, result);

    } else {
        Serial.printf("[DATA] Unknown action: %s\n", action.c_str());
    }
}

} // namespace DataEngine
