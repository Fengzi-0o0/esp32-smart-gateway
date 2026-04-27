#ifndef PIN_CAPS_H
#define PIN_CAPS_H

#include <Arduino.h>

// ================================================================
//  ESP32-S3-N16R8 外部可用引脚能力表
//  数据来源：芯片引脚图
// ================================================================

struct PinCapability {
    uint8_t pin;
    bool    digitalIO;
    bool    pwm;
    bool    adc;
    uint8_t adcChannel;    // 0xFF = 无ADC
    bool    touch;
    const char* altFunc;   // 复用功能说明
};

static const PinCapability pinCaps[] = {
    // ---- 左侧引脚（从上到下）----
    // pin  digitalIO  pwm   adc   adcCh  touch  altFunc
    {  0,    true,     true, false, 0xFF,  false, "BOOT按钮, 低电平进下载模式" },
    {  1,    true,     true, true,  0,     true,  "ADC1_CH0, TOUCH1" },
    {  2,    true,     true, true,  1,     true,  "ADC1_CH1, TOUCH2" },
    {  3,    true,     true, true,  2,     true,  "JTAG, ADC1_CH2, TOUCH3" },
    {  4,    true,     true, true,  3,     true,  "ADC1_CH3, TOUCH4" },
    {  5,    true,     true, true,  4,     true,  "ADC1_CH4, TOUCH5" },
    {  6,    true,     true, true,  5,     true,  "ADC1_CH5, TOUCH6" },
    {  7,    true,     true, true,  6,     true,  "ADC1_CH6, TOUCH7" },
    {  8,    true,     true, true,  7,     true,  "ADC1_CH7, TOUCH8" },
    {  9,    true,     true, true,  8,     true,  "ADC1_CH8, TOUCH9" },
    { 10,    true,     true, true,  9,     true,  "ADC1_CH9, TOUCH10" },
    { 11,    true,     true, true,  0,     true,  "ADC2_CH0, TOUCH11" },
    { 12,    true,     true, true,  1,     true,  "ADC2_CH1, TOUCH12" },
    { 13,    true,     true, true,  2,     true,  "ADC2_CH2, TOUCH13, FSPIQ" },
    { 14,    true,     true, true,  3,     true,  "ADC2_CH3, TOUCH14, FSPIWP" },
    { 15,    true,     true, true,  4,     false, "ADC2_CH4, U0RTS" },
    { 16,    true,     true, true,  5,     false, "ADC2_CH5, U0CTS" },
    { 17,    true,     true, true,  6,     false, "ADC2_CH6, U1TXD" },
    { 18,    true,     true, true,  7,     false, "ADC2_CH7, U1RXD, CLK_OUT3" },
    { 19,    true,     true, true,  8,     false, "ADC2_CH8, U1RTS, USB_D- (板载)" },
    { 20,    true,     true, true,  9,     false, "ADC2_CH9, U1CTS, USB_D+ (板载)" },
    { 21,    true,     true, false, 0xFF,  false, "RTC GPIO" },

    // ---- 右侧引脚（从下到上）----
    { 38,    true,     true, true,  15,    false, "ADC1_CH15, FSPIWP, SUBSPIWP" },
    { 39,    true,     true, true,  14,    false, "ADC1_CH14, MTCK" },
    { 40,    true,     true, true,  13,    false, "ADC1_CH13, MTDO, CLK_OUT2" },
    { 41,    true,     true, true,  12,    false, "ADC1_CH12, MTDI, CLK_OUT3" },
    { 42,    true,     true, true,  11,    false, "ADC1_CH11, MTMS" },
    { 43,    true,     true, false, 0xFF,  false, "U0TXD (串口发送)" },
    { 44,    true,     true, false, 0xFF,  false, "U0RXD (串口接收)" },
    { 45,    true,     true, false, 0xFF,  false, "无特殊功能" },
    { 46,    true,     true, false, 0xFF,  false, "LOG (建议浮空或输出)" },
    { 47,    true,     true, false, 0xFF,  false, "SPICLK_P (板载PSRAM)" },
    { 48,    true,     true, false, 0xFF,  false, "SPICLK_N (板载RGB LED)" },
};
static const int pinCapsCount = sizeof(pinCaps) / sizeof(pinCaps[0]);

// ================================================================
//  外部可用引脚白名单
//  注意：GPIO 33-37 为 Flash SPI 专用，绝对禁止用户使用
// ================================================================

static const uint8_t validExternalPins[] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21,
    38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48
};
static const int validExternalPinsCount = sizeof(validExternalPins) / sizeof(validExternalPins[0]);

// ================================================================
//  引脚验证函数
// ================================================================

inline bool isValidExternalPin(int pin) {
    for (int i = 0; i < validExternalPinsCount; i++) {
        if (validExternalPins[i] == pin) return true;
    }
    return false;
}

// 查找引脚能力描述
inline const PinCapability* findPinCap(int pin) {
    for (int i = 0; i < pinCapsCount; i++) {
        if (pinCaps[i].pin == pin) return &pinCaps[i];
    }
    return nullptr;
}

#endif
