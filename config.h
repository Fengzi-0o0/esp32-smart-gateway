#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <vector>
#include <WebServer.h>
#include <Preferences.h>

#define DEFAULT_AP_SSID     "esp32byQFQ"
#define DEFAULT_AP_PASS     "12345678"
#define DEFAULT_MQTT_PORT   8883
#define MAX_TOPICS          20
#define MAX_SENSORS         10
#define MAX_INPUTS          10
#define MAX_PINS_CTRL       20
#define MAX_TIMERS          10       // 新增
#define MAX_LOGIC_RULES     10       // 新增
#define MAX_I2C_OPS          20
#define MAX_ONEWIRE_OPS      10
#define MAX_UART_PORTS      2
#define MAX_SPI_PORTS       2
#define MAX_TOUCH_PINS      10
#define FIRMWARE_VERSION    "1.0.1"  // 版本号更新
#define UDP_DISCOVERY_PORT  4210
#define OTA_CHUNK_SIZE      1024

#define MQTT_OTA_TOPIC          "esp32/ota"
#define MQTT_OTA_REPORT_TOPIC   "esp32/ota/status"

struct TopicEntry {
    String topic;
    uint8_t qos;
};

struct SensorTask {
    int  pin;
    int  interval;
    int  type;
    bool enabled;
    String label;
    bool persistent;  // 新增
};

struct InputTask {
    int  pin;
    int  mode;
    int  lastValue;
    int  debounceMs;
    unsigned long lastChange;
    bool enabled;
    String label;
    bool persistent;  // 新增
};

// ========== 新增：定时器任务结构 ==========
struct TimerEntry {
    String id;
    String type;           // "interval", "once", "count"
    unsigned long interval; // 执行间隔 ms
    int count;             // 执行次数, -1=无限
    bool enabled;
    String commandsJson;   // JSON 数组: [{"cmd":"set","pin":12,"value":1},...]
    bool persistent;  // 新增：true=持久化，false=临时
};

// ========== 新增：逻辑规则结构 ==========
struct ConditionEntry {
    String source;  // "sensor" 或 "input"
    int    pin;
    String op;      // ">", "<", "=", "!=" (sensor); "high", "low" (input)
    float  value;   // 阈值
};

struct LogicRuleEntry {
    String id;
    String operator_;              // "and" / "or"
    std::vector<ConditionEntry> conditions;
    String actionsJson;            // JSON 数组
    bool enabled;
    unsigned long cooldown;        // 冷却时间 ms
    bool persistent;  // 新增：true=持久化，false=临时
};

// ========== 新增：I2C 操作记录 ==========
struct I2CEntry {
    int  sda;
    int  scl;
    bool enabled;
};

// ========== 新增：1-Wire 引脚记录 ==========
struct OneWireEntry {
    int  pin;
    bool enabled;
};

// ========== UART 串口配置 ==========
struct UartEntry {
    int  port;        // 1 或 2（UART0 为调试串口，禁止使用）
    int  txPin;
    int  rxPin;
    int  baud;
    int  dataBits;    // 5, 6, 7, 8
    int  stopBits;    // 1, 2
    int  parity;      // 0=none, 1=even, 2=odd
    bool enabled;
    bool listening;   // 是否开启持续监听
};

// ========== SPI 总线配置 ==========
struct SpiEntry {
    int  port;        // 2=FSPI, 3=HSPI（SPI0/1 为 Flash/PSRAM 专用）
    int  mosiPin;
    int  misoPin;
    int  sclkPin;
    int  speed;       // Hz
    int  mode;        // 0, 1, 2, 3
    bool enabled;
};

// ========== Touch 触摸引脚配置 ==========
struct TouchEntry {
    int    pin;
    int    threshold;     // 低于此值判定为触摸
    int    debounceMs;
    bool   enabled;
    String label;
    bool   persistent;
};



struct DeviceConfig {
    String staSsid;
    String staPass;
    bool   apHidden;
    String mqttHost;
    uint16_t mqttPort;
    String mqttUser;
    String mqttPass;
    String mqttClientId;
    bool   mqttSsl;
    bool   mqttEnabled;  // ← 新增：MQTT 连接开关
    String deviceName;
    std::vector<TopicEntry> subTopics;
    std::vector<TopicEntry> pubTopics;
    std::vector<SensorTask> sensors;
    std::vector<InputTask>  inputs;
    std::vector<TimerEntry> timers;          // 新增
    std::vector<LogicRuleEntry> logicRules;  // 新增
    std::vector<I2CEntry>      i2cConfigs;     // 新增
    std::vector<OneWireEntry>  owConfigs;      // 新增
    std::vector<UartEntry>     uartConfigs;
    std::vector<SpiEntry>      spiConfigs;
    std::vector<TouchEntry>    touchPins;


    int    batchInterval;  // 新增: 批量上报间隔秒, 0=禁用

    void load();
    void save();
    void reset();
};

String getUniqueApSsid();
String getDeviceId();

extern WebServer     server;
extern DeviceConfig  config;
extern volatile bool otaInProgress;
extern const char* OTA_VERIFY_TAG;

#endif
