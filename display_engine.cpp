#include "display_engine.h"
#include "display_font.h"
#include "config.h"
#include "mqtt_client.h"
#include "script_engine.h"
#include "pin_caps.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================================================================
//  I2C 冲突检测变量（�?i2c_engine.cpp 访问�?
// ================================================================
namespace DisplayEngine {
    int displaySda = -1;
    int displayScl = -1;
}

// ================================================================
//  内部状�?
// ================================================================
static Adafruit_SSD1306 *oled = nullptr;
static bool initialized = false;
static int screenW = 128;
static int screenH = 64;
static int screenAddr = 0x3C;

// 脏页追踪
static uint8_t dirtyPages = 0;
static uint8_t lastFrameBuf[1024];

// 自适应帧率
static unsigned long baseInterval = 50;    // 用户设定的基础间隔
static unsigned long flushInterval = 50;   // 当前实际间隔
static unsigned long lastFlushTime = 0;
static int idleCount = 0;
static unsigned long flushCount = 0;
static unsigned long lastFlushCost = 0;

// ================================================================
//  工具函数
// ================================================================

static void markDirty(int y, int h) {
    int pageStart = y / 8;
    int pageEnd = (y + h - 1) / 8;
    for (int p = pageStart; p <= pageEnd && p < (screenH / 8); p++)
        dirtyPages |= (1 << p);
}

static void markAllDirty() {
    dirtyPages = 0xFF;
}

// 变量插值：替换 $V:varname
static String interpolateVars(const String &input) {
    String result = input;
    int searchFrom = 0;
    int guard = 0;
    while (guard++ < 20) {
        int pos = result.indexOf("$V:", searchFrom);
        if (pos < 0) break;

        String varName = "";
        for (int i = pos + 3; i < (int)result.length(); i++) {
            char c = result.charAt(i);
            if (isAlphaNumeric(c) || c == '_') varName += c;
            else break;
        }
        if (varName.length() > 0) {
            float val = ScriptEngine::getVar(varName, 0);
            String valStr;
            // 如果是整数则显示整数，否则保�?位小�?
            if (val == (int)val) valStr = String((int)val);
            else valStr = String(val, 1);
            result = result.substring(0, pos) + valStr + result.substring(pos + 3 + varName.length());
            searchFrom = pos + valStr.length();
        } else {
            searchFrom = pos + 3;
        }
    }
    return result;
}

// UTF-8 解码单个字符，返�?Unicode 码点和字节数
static uint32_t utf8Decode(const char *s, int *bytesUsed) {
    uint8_t c = (uint8_t)s[0];
    if (c < 0x80) { *bytesUsed = 1; return c; }
    if ((c & 0xE0) == 0xC0) { *bytesUsed = 2; return ((c & 0x1F) << 6) | (s[1] & 0x3F); }
    if ((c & 0xF0) == 0xE0) { *bytesUsed = 3; return ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); }
    *bytesUsed = 1; return c;
}

// Unicode �?GB2312 简易转换（覆盖常用汉字�?
static uint16_t unicodeToGb2312(uint32_t unicode) {
    // ASCII
    if (unicode < 0x80) return (uint16_t)unicode;

    // 查表：常�?Unicode �?GB2312 映射
    // 这里用一个简单的映射表覆盖方案中列出的字�?
    static const struct { uint32_t uni; uint16_t gb; } map[] = {
        {0x6E29, 0xCEC2}, // �?
        {0x5EA6, 0xB6C8}, // �?
        {0x6E7F, 0xCAAA}, // �?
        {0x72B6, 0xD7B4}, // �?
        {0x6001, 0xCCAC}, // �?
        {0x5F00, 0xBFAA}, // 开
        {0x5173, 0xB9D8}, // �?
        {0x8BBE, 0xC9E8}, // �?
        {0x7F6E, 0xD6C3}, // �?
        {0x65F6, 0xCAB1}, // �?
        {0x9AD8, 0xB8DF}, // �?
        {0x4F4E, 0xB5CD}, // �?
        {0x7CFB, 0xCFB5}, // �?
        {0x7EDF, 0xCDB3}, // �?
        {0x8FD0, 0xD4CB}, // �?
        {0x884C, 0xD0D0}, // �?
        {0x505C, 0xCDA3}, // �?
        {0x5728, 0xD4DA}, // �?
        {0x7EBF, 0xCFDF}, // �?
        {0x79BB, 0xC0EB}, // �?
        {0x7F51, 0xCDF8}, // �?
        {0x4FE1, 0xD0C5}, // �?
        {0x53F7, 0xBAC5}, // �?
        {0x7535, 0xB5E7}, // �?
        {0x6D41, 0xC1F7}, // �?
        {0x529F, 0xB9A6}, // �?
        {0x5907, 0xB1B8}, // �?
        {0x540D, 0xC3FB}, // �?
        {0x79F0, 0xB3C6}, // �?
        {0x81EA, 0xD7D4}, // �?
        {0x52A8, 0xB6AF}, // �?
        {0x65B0, 0xD0C2}, // �?
        {0x6B63, 0xD5FD}, // �?
        {0x5E38, 0xB3A3}, // �?
    };
    for (int i = 0; i < (int)(sizeof(map)/sizeof(map[0])); i++) {
        if (map[i].uni == unicode) return map[i].gb;
    }
    return 0; // 未找�?
}

// 查找中文字模
static const uint8_t* findCnBitmap(uint16_t gb) {
    for (int i = 0; i < CN_CHAR_COUNT; i++) {
        if (pgm_read_word(&FONT_CN_16X16[i].code) == gb)
            return FONT_CN_16X16[i].data;
    }
    return nullptr;
}

// 绘制单个中文字符�?6×16�?
static void drawChineseChar(int x, int y, uint16_t gb, uint16_t color) {
    const uint8_t *bmp = findCnBitmap(gb);
    if (!bmp) {
        // 未找到字模，画方框占�?
        if (oled) oled->drawRect(x, y, 16, 16, color);
        return;
    }
    for (int row = 0; row < 16; row++) {
        uint8_t b0 = pgm_read_byte(&bmp[row * 2]);
        uint8_t b1 = pgm_read_byte(&bmp[row * 2 + 1]);
        for (int col = 0; col < 8; col++) {
            if (b0 & (1 << (7 - col)))
                oled->drawPixel(x + col, y + row, color);
        }
        for (int col = 0; col < 8; col++) {
            if (b1 & (1 << (7 - col)))
                oled->drawPixel(x + 8 + col, y + row, color);
        }
    }
}

// ================================================================
//  绘图 API
// ================================================================

static void drawText(JsonDocument &doc) {
    if (!oled) return;
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    int sz = doc["size"] | 1;
    String text = doc["text"] | String("");
    int color = doc["color"] | 1;
    bool wrap = doc["wrap"] | false;

    // 变量插�?
    text = interpolateVars(text);

    oled->setTextSize(sz);
    oled->setTextColor(color ? WHITE : BLACK);

    int curX = x;
    int curY = y;
    int charW = (sz == 2) ? 12 : 6;
    int charH = (sz == 2) ? 16 : 8;
    int cnW = 16; // 中文字符固定16像素�?

    const char *str = text.c_str();
    int len = text.length();
    int i = 0;

    while (i < len) {
        int bytesUsed = 0;
        uint32_t ch = utf8Decode(str + i, &bytesUsed);

        if (ch < 0x80) {
            // ASCII 字符
            if (wrap && curX + charW > screenW) {
                curX = x;
                curY += charH;
            }
            oled->setCursor(curX, curY);
            oled->write((char)ch);
            curX += charW;
        } else {
            // 中文字符
            uint16_t gb = unicodeToGb2312(ch);
            if (wrap && curX + cnW > screenW) {
                curX = x;
                curY += charH;
            }
            if (gb != 0) {
                drawChineseChar(curX, curY, gb, color ? WHITE : BLACK);
            } else {
                // 未知字符，画占位�?
                oled->drawRect(curX, curY, cnW, charH, color ? WHITE : BLACK);
            }
            curX += cnW;
        }
        i += bytesUsed;
    }

    // 标记受影响的脏页
    int totalH = curY - y + charH;
    markDirty(y, totalH);
}

static void drawRectCmd(JsonDocument &doc) {
    if (!oled) return;
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    int w = doc["w"] | 50;
    int h = doc["h"] | 20;
    bool fill = doc["fill"] | false;
    int color = doc["color"] | 1;
    uint16_t c = color ? WHITE : BLACK;

    if (fill) oled->fillRect(x, y, w, h, c);
    else      oled->drawRect(x, y, w, h, c);
    markDirty(y, h);
}

static void drawLineCmd(JsonDocument &doc) {
    if (!oled) return;
    int x1 = doc["x1"] | 0;
    int y1 = doc["y1"] | 0;
    int x2 = doc["x2"] | 127;
    int y2 = doc["y2"] | 63;
    int color = doc["color"] | 1;
    oled->drawLine(x1, y1, x2, y2, color ? WHITE : BLACK);
    markDirty(min(y1, y2), abs(y2 - y1) + 1);
}

static void drawHlineCmd(JsonDocument &doc) {
    if (!oled) return;
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    int w = doc["w"] | 128;
    int color = doc["color"] | 1;
    oled->drawFastHLine(x, y, w, color ? WHITE : BLACK);
    markDirty(y, 1);
}

static void drawPixelCmd(JsonDocument &doc) {
    if (!oled) return;
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    int color = doc["color"] | 1;
    oled->drawPixel(x, y, color ? WHITE : BLACK);
    markDirty(y, 1);
}

static void drawProgressCmd(JsonDocument &doc) {
    if (!oled) return;
    int x = doc["x"] | 0;
    int y = doc["y"] | 56;
    int w = doc["w"] | 128;
    int h = doc["h"] | 8;

    // value 支持数字和字符串（含 $V: 变量�?
    float value = 0;
    if (doc["value"].is<int>()) {
        value = (float)doc["value"].as<int>();
    } else if (doc["value"].is<float>()) {
        value = doc["value"].as<float>();
    } else {
        String valStr = doc["value"] | String("0");
        valStr = interpolateVars(valStr);
        value = valStr.toFloat();
    }
    float maxVal = doc["max"] | 100.0f;

    if (maxVal <= 0) maxVal = 100;

    // 外框
    oled->drawRect(x, y, w, h, WHITE);
    // 内部填充
    int fillW = (int)((float)(w - 2) * value / maxVal);
    if (fillW > w - 2) fillW = w - 2;
    if (fillW > 0)
        oled->fillRect(x + 1, y + 1, fillW, h - 2, WHITE);
    // 清除填充外区�?
    if (fillW < w - 2)
        oled->fillRect(x + 1 + fillW, y + 1, w - 2 - fillW, h - 2, BLACK);

    markDirty(y, h);
}

static void drawBitmapCmd(JsonDocument &doc) {
    if (!oled) return;
    int x = doc["x"] | 0;
    int y = doc["y"] | 0;
    int w = doc["w"] | 32;
    int h = doc["h"] | 32;

    if (doc.containsKey("data")) {
        JsonArray arr = doc["data"].as<JsonArray>();
        int expectedBytes = (w * h + 7) / 8;
        uint8_t *buf = (uint8_t *)malloc(expectedBytes);
        if (!buf) return;
        int i = 0;
        for (JsonVariant v : arr) {
            if (i >= expectedBytes) break;
            buf[i++] = (uint8_t)v.as<int>();
        }
        oled->drawBitmap(x, y, buf, w, h, WHITE);
        free(buf);
    }
    else if (doc.containsKey("base64")) {
        // Base64 解码（简化版�?
        String b64 = doc["base64"].as<String>();
        int expectedBytes = (w * h + 7) / 8;
        uint8_t *buf = (uint8_t *)malloc(expectedBytes);
        if (!buf) return;
        // 简�?Base64 解码
        const char *b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        int outIdx = 0;
        uint32_t acc = 0;
        int bits = 0;
        for (int i = 0; i < (int)b64.length() && outIdx < expectedBytes; i++) {
            char c = b64.charAt(i);
            if (c == '=') break;
            const char *p = strchr(b64chars, c);
            if (!p) continue;
            acc = (acc << 6) | (p - b64chars);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                buf[outIdx++] = (acc >> bits) & 0xFF;
            }
        }
        oled->drawBitmap(x, y, buf, w, h, WHITE);
        free(buf);
    }

    markDirty(y, h);
}

// ================================================================
//  局部刷新（智能脏页 + 自适应帧率�?
// ================================================================

static void flushDirty(bool force = false) {
    if (!oled || !initialized) return;

    unsigned long now = millis();
    if (!force && now - lastFlushTime < flushInterval) return;

    unsigned long start = micros();
    bool hasDirty = false;
    int pagesPerScreen = screenH / 8;

    uint8_t *buf = oled->getBuffer(); 

    for (int page = 0; page < pagesPerScreen; page++) {
        if (!force && !(dirtyPages & (1 << page))) continue;
        int offset = page * screenW;

        if (!force) {
            // 智能脏页：对比当前帧和上一�?
            if (memcmp(&buf[offset], &lastFrameBuf[offset], screenW) == 0)
                continue;
        }

        // 设置页地址和列地址
        Wire.beginTransmission(screenAddr);
        Wire.write(0x00);            // 命令模式
        Wire.write(0xB0 | page);     // 页地址
        Wire.write(0x00);            // 列起始低4�?
        Wire.write(0x10);            // 列起始高4�?
        Wire.endTransmission();

        // 发�?28字节数据
        Wire.beginTransmission(screenAddr);
        Wire.write(0x40);            // 数据模式
        Wire.write(&buf[offset], screenW);
        Wire.endTransmission();

        memcpy(&lastFrameBuf[offset], &buf[offset], screenW);
        hasDirty = true;
    }
    dirtyPages = 0;

    unsigned long costUs = micros() - start;
    lastFlushCost = costUs / 1000; // 转为ms
    if (lastFlushCost < 1) lastFlushCost = 1;

    // ===== 自适应帧率 =====
    unsigned long costMs = lastFlushCost;

    if (force) {
        flushInterval = baseInterval;
        idleCount = 0;
    } else if (hasDirty) {
        idleCount = 0;
        // 间隔 = max(基础间隔, 耗时×8)，保证占空比 < 12.5%
        flushInterval = max(baseInterval, costMs * 8);
    } else {
        idleCount++;
        // 无变�?�?逐步延长，上�?2000ms
        flushInterval = min(baseInterval * (1UL << min(idleCount, 5)), 2000UL);
    }

    lastFlushTime = millis();
    flushCount++;
}

// ================================================================
//  SSD1306 初始化命令序列（库的 begin() 已包含，此函数备用）
// ================================================================

static bool initDisplay(int sda, int scl, int addr, int w, int h, bool flip, int contrast) {
    // 创建实例
    if (oled) {
        delete oled;
        oled = nullptr;
    }

    oled = new Adafruit_SSD1306(w, h, &Wire, -1);
    if (!oled) {
        Serial.println("[DISPLAY] Failed to allocate");
        return false;
    }

    // Wire 初始�?
    Wire.begin(sda, scl);
    Wire.setClock(400000);

    // SSD1306 初始�?
    if (!oled->begin(SSD1306_SWITCHCAPVCC, addr)) {
        Serial.println("[DISPLAY] SSD1306 init failed");
        delete oled;
        oled = nullptr;
        return false;
    }



    oled->ssd1306_command(SSD1306_SETCONTRAST);
    oled->ssd1306_command(contrast);

    oled->clearDisplay();
    oled->display(); // 首次全屏刷新

    // 记录状�?
    screenW = w;
    screenH = h;
    screenAddr = addr;

    // 记录引脚�?I2C 冲突检�?
    DisplayEngine::displaySda = sda;
    DisplayEngine::displayScl = scl;

    // 初始�?lastFrameBuf
    memset(lastFrameBuf, 0, sizeof(lastFrameBuf));
    markAllDirty();
    dirtyPages = 0;

    initialized = true;
    Serial.printf("[DISPLAY] OK: %dx%d addr=0x%02X sda=%d scl=%d\n",
                  w, h, addr, sda, scl);
    return true;
}

// ================================================================
//  命令分发
// ================================================================

namespace DisplayEngine {

void init() {
    initialized = false;
    oled = nullptr;
    displaySda = -1;
    displayScl = -1;
    Serial.println("[DISPLAY] Engine initialized (lazy)");
}

void loop() {
    // 自适应刷新：定时检查是否需要刷�?
    // if (initialized && dirtyPages) {
    //     flushDirty(false);
    // }
}

void handleCommand(JsonDocument &doc) {
    String action = doc["action"] | String("");

    // ===== init =====
    if (action == "init") {
        int sda = doc["sda"] | -1;
        int scl = doc["scl"] | -1;
        if (sda < 0 || scl < 0) {
            Serial.println("[DISPLAY] init: sda/scl required");
            return;
        }
        if (!isValidExternalPin(sda)) { Serial.printf("[DISPLAY] REJECTED: sda=%d\n", sda); return; }
        if (!isValidExternalPin(scl)) { Serial.printf("[DISPLAY] REJECTED: scl=%d\n", scl); return; }

        int addr = doc["address"] | 0x3C;
        int w = doc["width"] | 128;
        int h = doc["height"] | 64;
        bool flip = doc["flip"] | false;
        int contrast = doc["contrast"] | 128;
        baseInterval = doc["minFlushMs"] | 50;
        flushInterval = baseInterval;
        bool persist = doc["persistent"] | false;

        if (initDisplay(sda, scl, addr, w, h, flip, contrast)) {
            // 持久�?
            if (persist) {
                config.display.sdaPin = sda;
                config.display.sclPin = scl;
                config.display.address = addr;
                config.display.width = w;
                config.display.height = h;
                config.display.flip = flip;
                config.display.contrast = contrast;
                config.display.minFlushMs = (int)baseInterval;
                config.display.enabled = true;
                config.markDirty();
            }

            JsonDocument resp;
            resp["type"] = "display_init";
            resp["deviceId"] = getDeviceId();
            resp["width"] = w;
            resp["height"] = h;
            resp["address"] = addr;
            resp["sda"] = sda;
            resp["scl"] = scl;
            String p;
            serializeJson(resp, p);
            if (MqttClient::isConnected() && config.pubTopics.size() > 0)
                MqttClient::publish(config.pubTopics[0].topic, p);
        }
    }

    // ===== clear =====
    else if (action == "clear") {
        if (!initialized) return;
        oled->clearDisplay();
        markAllDirty();
    }

    // ===== clear_rect =====
    else if (action == "clear_rect") {
        if (!initialized) return;
        int x = doc["x"] | 0;
        int y = doc["y"] | 0;
        int w = doc["w"] | screenW;
        int h = doc["h"] | screenH;
        oled->fillRect(x, y, w, h, BLACK);
        markDirty(y, h);
    }

    // ===== text =====
    else if (action == "text") {
        if (!initialized) return;
        drawText(doc);
    }

    // ===== rect =====
    else if (action == "rect") {
        if (!initialized) return;
        drawRectCmd(doc);
    }

    // ===== line =====
    else if (action == "line") {
        if (!initialized) return;
        drawLineCmd(doc);
    }

    // ===== hline =====
    else if (action == "hline") {
        if (!initialized) return;
        drawHlineCmd(doc);
    }

    // ===== pixel =====
    else if (action == "pixel") {
        if (!initialized) return;
        drawPixelCmd(doc);
    }

    // ===== progress =====
    else if (action == "progress") {
        if (!initialized) return;
        drawProgressCmd(doc);
    }

    // ===== bitmap =====
    else if (action == "bitmap") {
        if (!initialized) return;
        drawBitmapCmd(doc);
    }

    // ===== flush =====
    else if (action == "flush") {
        if (!initialized) return;
        bool force = doc["force"] | false;
        flushDirty(force);
    }

    // ===== on =====
    else if (action == "on") {
        if (!initialized) return;
        oled->ssd1306_command(SSD1306_DISPLAYON);
    }

    // ===== off =====
    else if (action == "off") {
        if (!initialized) return;
        oled->ssd1306_command(SSD1306_DISPLAYOFF);
    }

    // ===== contrast =====
    else if (action == "contrast") {
        if (!initialized) return;
        int val = doc["value"] | 128;
        oled->ssd1306_command(SSD1306_SETCONTRAST);
        oled->ssd1306_command(val & 0xFF);
    }

    // ===== invert =====
    else if (action == "invert") {
        if (!initialized) return;
        bool en = doc["enabled"] | false;
        oled->invertDisplay(en);
    }

    // ===== flip =====
    else if (action == "flip") {
        if (!initialized) return;
        bool en = doc["enabled"] | false;
        if (en) {
            // 旋转180°
            oled->ssd1306_command(SSD1306_SEGREMAP);        // 0xA0
            oled->ssd1306_command(SSD1306_COMSCANINC);      // 0xC0
        } else {
            // 恢复正常（Adafruit默认方向�?
            oled->ssd1306_command(SSD1306_SEGREMAP | 0x01); // 0xA1
            oled->ssd1306_command(SSD1306_COMSCANDEC);      // 0xC8
        }
    }


    // ===== scene =====
    else if (action == "scene") {
        if (!initialized) return;
        if (!doc.containsKey("commands")) {
            Serial.println("[DISPLAY] scene: commands required");
            return;
        }
        JsonArray cmds = doc["commands"].as<JsonArray>();
        for (JsonObject cmd : cmds) {
            JsonDocument sub;
            sub.set(cmd);
            handleCommand(sub);
        }
    }

    // ===== status =====
    else if (action == "status") {
        JsonDocument resp;
        resp["type"] = "display_status";
        resp["deviceId"] = getDeviceId();
        resp["initialized"] = initialized;
        resp["sda"] = DisplayEngine::displaySda;
        resp["scl"] = DisplayEngine::displayScl;
        resp["address"] = screenAddr;
        resp["width"] = screenW;
        resp["height"] = screenH;
        resp["pages"] = screenH / 8;
        resp["dirtyPages"] = dirtyPages;
        resp["flushCount"] = flushCount;
        resp["lastFlushMs"] = lastFlushCost;
        resp["currentInterval"] = flushInterval;
        resp["baseInterval"] = baseInterval;
        resp["idleCount"] = idleCount;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
    }

    // ===== 未知 =====
    else {
        Serial.printf("[DISPLAY] Unknown action: %s\n", action.c_str());
    }
}

} // namespace DisplayEngine
