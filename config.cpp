#include "config.h"
#include <Preferences.h>
#include <WiFi.h>

static Preferences prefs;

String getUniqueApSsid() {
  char suffix[16];

  uint64_t efuseMac = ESP.getEfuseMac();
  uint8_t* mac = (uint8_t*)&efuseMac;

  // 检查是否全为 0 或全为 0xFF（无效 MAC）
  bool allZero = true;
  bool allFF = true;
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0x00) allZero = false;
    if (mac[i] != 0xFF) allFF = false;
  }

  if (!allZero && !allFF) {
    // 用全部 6 字节，保证唯一
    snprintf(suffix, sizeof(suffix), "-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(DEFAULT_AP_SSID) + suffix;
  }

  // MAC 无效，用随机 ID
  prefs.begin("uid", false);
  bool exists = prefs.getBool("init", false);

  if (!exists) {
    uint32_t randId = (uint32_t)(esp_random() & 0xFFFFFF);
    prefs.putUInt("rid", randId);
    prefs.putBool("init", true);
    Serial.printf("[CONFIG] Generated random ID: %06X\n", randId);
  }

  uint32_t randId = prefs.getUInt("rid", 0);
  prefs.end();

  snprintf(suffix, sizeof(suffix), "-R%06X", randId & 0xFFFFFF);
  return String(DEFAULT_AP_SSID) + suffix;
}

String getDeviceId() {
  String ssid = getUniqueApSsid();
  int pos = ssid.lastIndexOf('-');
  if (pos >= 0) {
    return ssid.substring(pos + 1);
  }
  return ssid;
}

void DeviceConfig::load() {
  _dirty = false;
  prefs.begin("cfg", true);

  staSsid = prefs.getString("s_ssid", "");
  staPass = prefs.getString("s_pass", "");
  apHidden = prefs.getBool("ap_hidden", false);
  mqttHost = prefs.getString("m_host", "");
  mqttPort = prefs.getUShort("m_port", DEFAULT_MQTT_PORT);
  mqttUser = prefs.getString("m_user", "");
  mqttPass = prefs.getString("m_pass", "");
  mqttClientId = prefs.getString("m_cid", "");
  mqttSsl = prefs.getBool("m_ssl", true);
  mqttEnabled = prefs.getBool("m_en", true);
  deviceName = prefs.getString("d_name", "");

  batchInterval = prefs.getInt("batch_int", 0);

  subTopics.clear();
  uint8_t sc = prefs.getUChar("s_cnt", 0);
  for (uint8_t i = 0; i < sc && i < MAX_TOPICS; i++) {
    TopicEntry t;
    t.topic = prefs.getString(("s_t" + String(i)).c_str(), "");
    t.qos = prefs.getUChar(("s_q" + String(i)).c_str(), 0);
    if (t.topic.length() > 0) subTopics.push_back(t);
  }

  pubTopics.clear();
  uint8_t pc = prefs.getUChar("p_cnt", 0);
  for (uint8_t i = 0; i < pc && i < MAX_TOPICS; i++) {
    TopicEntry t;
    t.topic = prefs.getString(("p_t" + String(i)).c_str(), "");
    t.qos = prefs.getUChar(("p_q" + String(i)).c_str(), 0);
    if (t.topic.length() > 0) pubTopics.push_back(t);
  }

  sensors.clear();
  uint8_t snrCnt = prefs.getUChar("snr_cnt", 0);
  for (uint8_t i = 0; i < snrCnt && i < MAX_SENSORS; i++) {
    SensorTask s;
    String p = "snr" + String(i) + "_";
    s.pin = prefs.getInt((p + "pin").c_str(), -1);
    s.interval = prefs.getInt((p + "int").c_str(), 5000);
    s.type = prefs.getInt((p + "typ").c_str(), 0);
    s.enabled = prefs.getBool((p + "en").c_str(), false);
    s.label = prefs.getString((p + "lbl").c_str(), "");
    s.persistent = prefs.getBool((p + "per").c_str(), true);  // 新增
    if (s.pin >= 0) sensors.push_back(s);
  }

  inputs.clear();
  uint8_t inCnt = prefs.getUChar("in_cnt", 0);
  for (uint8_t i = 0; i < inCnt && i < MAX_INPUTS; i++) {
    InputTask t;
    String p = "in" + String(i) + "_";
    t.pin = prefs.getInt((p + "pin").c_str(), -1);
    t.mode = prefs.getInt((p + "mode").c_str(), INPUT_PULLUP);
    t.debounceMs = prefs.getInt((p + "deb").c_str(), 50);
    t.enabled = prefs.getBool((p + "en").c_str(), false);
    t.label = prefs.getString((p + "lbl").c_str(), "");
    t.persistent = prefs.getBool((p + "per").c_str(), true);  // 新增
    t.lastValue = -1;
    t.lastChange = 0;
    if (t.pin >= 0) inputs.push_back(t);
  }

  timers.clear();
  uint8_t tmrCnt = prefs.getUChar("tmr_cnt", 0);
  for (uint8_t i = 0; i < tmrCnt && i < MAX_TIMERS; i++) {
    TimerEntry te;
    String p = "tmr" + String(i) + "_";
    te.id = prefs.getString((p + "id").c_str(), "");
    te.type = prefs.getString((p + "type").c_str(), "interval");
    te.interval = prefs.getULong((p + "int").c_str(), 5000);
    te.count = prefs.getInt((p + "cnt").c_str(), -1);
    te.enabled = prefs.getBool((p + "en").c_str(), false);
    te.commandsJson = prefs.getString((p + "cmds").c_str(), "[]");
    te.persistent = prefs.getBool((p + "per").c_str(), true);
    if (te.id.length() > 0) timers.push_back(te);
  }

  logicRules.clear();
  uint8_t lgrCnt = prefs.getUChar("lgr_cnt", 0);
  for (uint8_t i = 0; i < lgrCnt && i < MAX_LOGIC_RULES; i++) {
    LogicRuleEntry lr;
    String p = "lgr" + String(i) + "_";
    lr.id = prefs.getString((p + "id").c_str(), "");
    lr.operator_ = prefs.getString((p + "op").c_str(), "and");
    lr.enabled = prefs.getBool((p + "en").c_str(), false);
    lr.cooldown = prefs.getULong((p + "cd").c_str(), 1000);
    lr.actionsJson = prefs.getString((p + "act").c_str(), "[]");
    lr.persistent = prefs.getBool((p + "per").c_str(), true);  // 新增

    uint8_t condCnt = prefs.getUChar((p + "ccnt").c_str(), 0);
    for (uint8_t j = 0; j < condCnt; j++) {
      ConditionEntry ce;
      String cp = p + "c" + String(j) + "_";
      ce.source = prefs.getString((cp + "src").c_str(), "sensor");
      ce.pin = prefs.getInt((cp + "pin").c_str(), -1);
      ce.op = prefs.getString((cp + "op").c_str(), ">");
      ce.value = prefs.getFloat((cp + "val").c_str(), 0);
      lr.conditions.push_back(ce);
    }

    if (lr.id.length() > 0) logicRules.push_back(lr);
  }

  i2cConfigs.clear();
  uint8_t i2cCnt = prefs.getUChar("i2c_cnt", 0);
  for (uint8_t i = 0; i < i2cCnt && i < MAX_I2C_OPS; i++) {
    I2CEntry e;
    String p = "i2c" + String(i) + "_";
    e.sda = prefs.getInt((p + "sda").c_str(), -1);
    e.scl = prefs.getInt((p + "scl").c_str(), -1);
    e.enabled = prefs.getBool((p + "en").c_str(), false);
    if (e.sda >= 0 && e.scl >= 0) i2cConfigs.push_back(e);
  }

  owConfigs.clear();
  uint8_t owCnt = prefs.getUChar("ow_cnt", 0);
  for (uint8_t i = 0; i < owCnt && i < MAX_ONEWIRE_OPS; i++) {
    OneWireEntry e;
    String p = "ow" + String(i) + "_";
    e.pin = prefs.getInt((p + "pin").c_str(), -1);
    e.enabled = prefs.getBool((p + "en").c_str(), false);
    if (e.pin >= 0) owConfigs.push_back(e);
  }

    // UART
  uartConfigs.clear();
  uint8_t uartCnt = prefs.getUChar("uart_cnt", 0);
  for (uint8_t i = 0; i < uartCnt && i < MAX_UART_PORTS; i++) {
    UartEntry e;
    String p = "uart" + String(i) + "_";
    e.port      = prefs.getInt((p + "pt").c_str(), 1);
    e.txPin     = prefs.getInt((p + "tx").c_str(), -1);
    e.rxPin     = prefs.getInt((p + "rx").c_str(), -1);
    e.baud      = prefs.getInt((p + "bd").c_str(), 9600);
    e.dataBits  = prefs.getInt((p + "db").c_str(), 8);
    e.stopBits  = prefs.getInt((p + "sb").c_str(), 1);
    e.parity    = prefs.getInt((p + "pr").c_str(), 0);
    e.enabled   = prefs.getBool((p + "en").c_str(), false);
    e.listening = prefs.getBool((p + "ls").c_str(), false);
    if (e.txPin >= 0 || e.rxPin >= 0) uartConfigs.push_back(e);
  }

  // SPI
  spiConfigs.clear();
  uint8_t spiCnt = prefs.getUChar("spi_cnt", 0);
  for (uint8_t i = 0; i < spiCnt && i < MAX_SPI_PORTS; i++) {
    SpiEntry e;
    String p = "spi" + String(i) + "_";
    e.port    = prefs.getInt((p + "pt").c_str(), 2);
    e.mosiPin = prefs.getInt((p + "mo").c_str(), -1);
    e.misoPin = prefs.getInt((p + "mi").c_str(), -1);
    e.sclkPin = prefs.getInt((p + "sc").c_str(), -1);
    e.speed   = prefs.getInt((p + "sp").c_str(), 1000000);
    e.mode    = prefs.getInt((p + "md").c_str(), 0);
    e.enabled = prefs.getBool((p + "en").c_str(), false);
    if (e.mosiPin >= 0 || e.sclkPin >= 0) spiConfigs.push_back(e);
  }

  // Touch
  touchPins.clear();
  uint8_t tchCnt = prefs.getUChar("tch_cnt", 0);
  for (uint8_t i = 0; i < tchCnt && i < MAX_TOUCH_PINS; i++) {
    TouchEntry e;
    String p = "tch" + String(i) + "_";
    e.pin        = prefs.getInt((p + "pin").c_str(), -1);
    e.threshold  = prefs.getInt((p + "thr").c_str(), 0);
    e.debounceMs = prefs.getInt((p + "db").c_str(), 50);
    e.enabled    = prefs.getBool((p + "en").c_str(), false);
    e.label      = prefs.getString((p + "lbl").c_str(), "");
    e.persistent = prefs.getBool((p + "per").c_str(), true);
    if (e.pin >= 0) touchPins.push_back(e);
  }

// ========== 新增：Display ==========
  display.sdaPin     = prefs.getInt("dsp_sda", -1);
  display.sclPin     = prefs.getInt("dsp_scl", -1);
  display.address    = prefs.getInt("dsp_addr", 0x3C);
  display.width      = prefs.getInt("dsp_w", 128);
  display.height     = prefs.getInt("dsp_h", 64);
  display.flip       = prefs.getBool("dsp_flip", false);
  display.contrast   = prefs.getInt("dsp_ctr", 128);
  display.minFlushMs = prefs.getInt("dsp_mfm", 50);
  display.enabled    = prefs.getBool("dsp_en", false);

  prefs.end();
  Serial.println("[CONFIG] Loaded from NVS");
}

void DeviceConfig::save() {
  prefs.begin("cfg", false);

  prefs.putString("s_ssid", staSsid);
  prefs.putString("s_pass", staPass);
  prefs.putBool("ap_hidden", apHidden);
  prefs.putString("m_host", mqttHost);
  prefs.putUShort("m_port", mqttPort);
  prefs.putString("m_user", mqttUser);
  prefs.putString("m_pass", mqttPass);
  prefs.putString("m_cid", mqttClientId);
  prefs.putBool("m_ssl", mqttSsl);
  prefs.putBool("m_en", mqttEnabled);
  prefs.putString("d_name", deviceName);
  prefs.putInt("batch_int", batchInterval);

    // UART
  prefs.putUChar("uart_cnt", uartConfigs.size());
  for (uint8_t i = 0; i < uartConfigs.size() && i < MAX_UART_PORTS; i++) {
    String p = "uart" + String(i) + "_";
    prefs.putInt((p + "pt").c_str(), uartConfigs[i].port);
    prefs.putInt((p + "tx").c_str(), uartConfigs[i].txPin);
    prefs.putInt((p + "rx").c_str(), uartConfigs[i].rxPin);
    prefs.putInt((p + "bd").c_str(), uartConfigs[i].baud);
    prefs.putInt((p + "db").c_str(), uartConfigs[i].dataBits);
    prefs.putInt((p + "sb").c_str(), uartConfigs[i].stopBits);
    prefs.putInt((p + "pr").c_str(), uartConfigs[i].parity);
    prefs.putBool((p + "en").c_str(), uartConfigs[i].enabled);
    prefs.putBool((p + "ls").c_str(), uartConfigs[i].listening);
  }

  // SPI
  prefs.putUChar("spi_cnt", spiConfigs.size());
  for (uint8_t i = 0; i < spiConfigs.size() && i < MAX_SPI_PORTS; i++) {
    String p = "spi" + String(i) + "_";
    prefs.putInt((p + "pt").c_str(), spiConfigs[i].port);
    prefs.putInt((p + "mo").c_str(), spiConfigs[i].mosiPin);
    prefs.putInt((p + "mi").c_str(), spiConfigs[i].misoPin);
    prefs.putInt((p + "sc").c_str(), spiConfigs[i].sclkPin);
    prefs.putInt((p + "sp").c_str(), spiConfigs[i].speed);
    prefs.putInt((p + "md").c_str(), spiConfigs[i].mode);
    prefs.putBool((p + "en").c_str(), spiConfigs[i].enabled);
  }

  // Touch
  prefs.putUChar("tch_cnt", touchPins.size());
  for (uint8_t i = 0; i < touchPins.size() && i < MAX_TOUCH_PINS; i++) {
    String p = "tch" + String(i) + "_";
    prefs.putInt((p + "pin").c_str(), touchPins[i].pin);
    prefs.putInt((p + "thr").c_str(), touchPins[i].threshold);
    prefs.putInt((p + "db").c_str(), touchPins[i].debounceMs);
    prefs.putBool((p + "en").c_str(), touchPins[i].enabled);
    prefs.putString((p + "lbl").c_str(), touchPins[i].label);
    prefs.putBool((p + "per").c_str(), touchPins[i].persistent);
  }

   // ========== 新增：Display ==========
  prefs.putInt("dsp_sda",   display.sdaPin);
  prefs.putInt("dsp_scl",   display.sclPin);
  prefs.putInt("dsp_addr",  display.address);
  prefs.putInt("dsp_w",     display.width);
  prefs.putInt("dsp_h",     display.height);
  prefs.putBool("dsp_flip", display.flip);
  prefs.putInt("dsp_ctr",   display.contrast);
  prefs.putInt("dsp_mfm",   display.minFlushMs);
  prefs.putBool("dsp_en",   display.enabled);


  prefs.putUChar("s_cnt", subTopics.size());
  for (uint8_t i = 0; i < subTopics.size() && i < MAX_TOPICS; i++) {
    prefs.putString(("s_t" + String(i)).c_str(), subTopics[i].topic);
    prefs.putUChar(("s_q" + String(i)).c_str(), subTopics[i].qos);
  }

  prefs.putUChar("p_cnt", pubTopics.size());
  for (uint8_t i = 0; i < pubTopics.size() && i < MAX_TOPICS; i++) {
    prefs.putString(("p_t" + String(i)).c_str(), pubTopics[i].topic);
    prefs.putUChar(("p_q" + String(i)).c_str(), pubTopics[i].qos);
  }

  prefs.putUChar("snr_cnt", sensors.size());
  for (uint8_t i = 0; i < sensors.size() && i < MAX_SENSORS; i++) {
    String p = "snr" + String(i) + "_";
    prefs.putInt((p + "pin").c_str(), sensors[i].pin);
    prefs.putInt((p + "int").c_str(), sensors[i].interval);
    prefs.putInt((p + "typ").c_str(), sensors[i].type);
    prefs.putBool((p + "en").c_str(), sensors[i].enabled);
    prefs.putString((p + "lbl").c_str(), sensors[i].label);
    prefs.putBool((p + "per").c_str(), sensors[i].persistent);  // 新增
  }

  prefs.putUChar("in_cnt", inputs.size());
  for (uint8_t i = 0; i < inputs.size() && i < MAX_INPUTS; i++) {
    String p = "in" + String(i) + "_";
    prefs.putInt((p + "pin").c_str(), inputs[i].pin);
    prefs.putInt((p + "mode").c_str(), inputs[i].mode);
    prefs.putInt((p + "deb").c_str(), inputs[i].debounceMs);
    prefs.putBool((p + "en").c_str(), inputs[i].enabled);
    prefs.putString((p + "lbl").c_str(), inputs[i].label);
    prefs.putBool((p + "per").c_str(), inputs[i].persistent);  // 新增
  }

  prefs.putUChar("tmr_cnt", timers.size());
  for (uint8_t i = 0; i < timers.size() && i < MAX_TIMERS; i++) {
    String p = "tmr" + String(i) + "_";
    prefs.putString((p + "id").c_str(), timers[i].id);
    prefs.putString((p + "type").c_str(), timers[i].type);
    prefs.putULong((p + "int").c_str(), timers[i].interval);
    prefs.putInt((p + "cnt").c_str(), timers[i].count);
    prefs.putBool((p + "en").c_str(), timers[i].enabled);
    prefs.putString((p + "cmds").c_str(), timers[i].commandsJson);
    prefs.putBool((p + "per").c_str(), timers[i].persistent);  // 新增
  }

  prefs.putUChar("lgr_cnt", logicRules.size());
  for (uint8_t i = 0; i < logicRules.size() && i < MAX_LOGIC_RULES; i++) {
    String p = "lgr" + String(i) + "_";
    prefs.putString((p + "id").c_str(), logicRules[i].id);
    prefs.putString((p + "op").c_str(), logicRules[i].operator_);
    prefs.putBool((p + "en").c_str(), logicRules[i].enabled);
    prefs.putULong((p + "cd").c_str(), logicRules[i].cooldown);
    prefs.putString((p + "act").c_str(), logicRules[i].actionsJson);
    prefs.putBool((p + "per").c_str(), logicRules[i].persistent); 

    prefs.putUChar((p + "ccnt").c_str(), logicRules[i].conditions.size());
    for (uint8_t j = 0; j < logicRules[i].conditions.size(); j++) {
      String cp = p + "c" + String(j) + "_";
      prefs.putString((cp + "src").c_str(), logicRules[i].conditions[j].source);
      prefs.putInt((cp + "pin").c_str(), logicRules[i].conditions[j].pin);
      prefs.putString((cp + "op").c_str(), logicRules[i].conditions[j].op);
      prefs.putFloat((cp + "val").c_str(), logicRules[i].conditions[j].value);
    }
  }

  prefs.putUChar("i2c_cnt", i2cConfigs.size());
  for (uint8_t i = 0; i < i2cConfigs.size() && i < MAX_I2C_OPS; i++) {
    String p = "i2c" + String(i) + "_";
    prefs.putInt((p + "sda").c_str(), i2cConfigs[i].sda);
    prefs.putInt((p + "scl").c_str(), i2cConfigs[i].scl);
    prefs.putBool((p + "en").c_str(), i2cConfigs[i].enabled);
  }

  prefs.putUChar("ow_cnt", owConfigs.size());
  for (uint8_t i = 0; i < owConfigs.size() && i < MAX_ONEWIRE_OPS; i++) {
    String p = "ow" + String(i) + "_";
    prefs.putInt((p + "pin").c_str(), owConfigs[i].pin);
    prefs.putBool((p + "en").c_str(), owConfigs[i].enabled);
  }


  prefs.end();
  _dirty = false;
  Serial.println("[CONFIG] Saved to NVS");
}

void DeviceConfig::reset() {
  prefs.begin("cfg", false);
  prefs.clear();
  prefs.end();
  staSsid = "";
  staPass = "";
  apHidden = false;
  mqttHost = "";
  mqttPort = DEFAULT_MQTT_PORT;
  mqttUser = "";
  mqttPass = "";
  mqttClientId = "";
  mqttSsl = true;
  mqttEnabled = true;
  deviceName = "";
  batchInterval = 0;
  subTopics.clear();
  pubTopics.clear();
  sensors.clear();
  inputs.clear();
  timers.clear();
  logicRules.clear();
  i2cConfigs.clear();
  owConfigs.clear();
  uartConfigs.clear();
  spiConfigs.clear();
  touchPins.clear();

  // ========== 新增：Display ==========
  display.sdaPin     = -1;
  display.sclPin     = -1;
  display.address    = 0x3C;
  display.width      = 128;
  display.height     = 64;
  display.flip       = false;
  display.contrast   = 128;
  display.minFlushMs = 50;
  display.enabled    = false;


  Serial.println("[CONFIG] Reset to defaults");
}
