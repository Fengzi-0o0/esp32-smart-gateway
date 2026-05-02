#include "web_server.h"
#include "web_page.h"
#include "config.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "timer_engine.h"
#include "logic_engine.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <WebSocketsServer.h>
#include "pin_caps.h"
#include "pins_page.h"
#include "monitor_page.h"
#include "cmd_builder_page.h"
#include "dual_channel.h"
#include <Preferences.h>



static WebSocketsServer wsOta(8080);

static void wsResultForward(uint8_t clientNum, const char *data) {
  wsOta.sendTXT(clientNum, data);
}


// ==================== 原有处理函数 ====================

static void handleRoot() {
  server.send_P(200, "text/html", MAIN_PAGE);
}

static void handlePinsPage() {
  server.send_P(200, "text/html", PINS_PAGE);
}


static void handleGetConfig() {
  JsonDocument doc;
  doc["staSsid"] = config.staSsid;
  doc["staPass"] = config.staPass;
  doc["apHidden"] = config.apHidden;
  doc["mqttHost"] = config.mqttHost;
  doc["mqttPort"] = config.mqttPort;
  doc["mqttUser"] = config.mqttUser;
  doc["mqttPass"] = config.mqttPass;
  doc["mqttClientId"] = config.mqttClientId;
  doc["mqttSsl"] = config.mqttSsl;
  doc["mqttEnabled"] = config.mqttEnabled;
  doc["deviceName"] = config.deviceName;
  doc["batchInterval"] = config.batchInterval;

  JsonArray subs = doc["subTopics"].to<JsonArray>();
  for (auto &t : config.subTopics) {
    JsonObject o = subs.add<JsonObject>();
    o["topic"] = t.topic;
    o["qos"] = t.qos;
  }

  JsonArray pubs = doc["pubTopics"].to<JsonArray>();
  for (auto &t : config.pubTopics) {
    JsonObject o = pubs.add<JsonObject>();
    o["topic"] = t.topic;
    o["qos"] = t.qos;
  }

  String json;
  serializeJson(doc, json);
  Serial.printf("[WEB] Config JSON: %d bytes\n", json.length());
  server.send(200, "application/json", json);
}

static void handleGetPresets() {
    Preferences prefs;
    prefs.begin("presets", true);
    uint8_t cnt = prefs.getUChar("cnt", 0);
    if (cnt > 20) cnt = 20;

    JsonDocument doc;
    JsonArray arr = doc["presets"].to<JsonArray>();

    for (uint8_t i = 0; i < cnt; i++) {
        String p = "p" + String(i) + "_";
        String name = prefs.getString((p + "nm").c_str(), "");
        String json = prefs.getString((p + "js").c_str(), "[]");
        if (name.length() > 0) {
            JsonObject o = arr.add<JsonObject>();
            o["id"] = i;
            o["name"] = name;
            JsonDocument blocksDoc;
            if (!deserializeJson(blocksDoc, json)) {
                o["blocks"] = blocksDoc;
            }
        }
    }
    prefs.end();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

static void handlePostPresets() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"ok\":false}");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        server.send(400, "application/json", "{\"ok\":false}");
        return;
    }

    String name = doc["name"] | String("");
    if (name.length() == 0) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"name required\"}");
        return;
    }

    Preferences prefs;
    prefs.begin("presets", false);
    uint8_t cnt = prefs.getUChar("cnt", 0);

    int existingIdx = -1;
    for (uint8_t i = 0; i < cnt && i < 20; i++) {
        if (prefs.getString(("p" + String(i) + "_nm").c_str(), "") == name) {
            existingIdx = i;
            break;
        }
    }

    int idx = (existingIdx >= 0) ? existingIdx : cnt;
    if (idx >= 20) {
        prefs.end();
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"max 20 presets\"}");
        return;
    }

    String p = "p" + String(idx) + "_";
    prefs.putString((p + "nm").c_str(), name);

    String blocksJson;
    serializeJson(doc["blocks"], blocksJson);
    prefs.putString((p + "js").c_str(), blocksJson);

    if (existingIdx < 0) {
        prefs.putUChar("cnt", cnt + 1);
    }
    prefs.end();

    JsonDocument resp;
    resp["ok"] = true;
    resp["id"] = idx;
    String response;
    serializeJson(resp, response);
    server.send(200, "application/json", response);
    Serial.printf("[PRESET] Saved '%s' (idx=%d, %d bytes)\n",
                  name.c_str(), idx, blocksJson.length());
}

static void handleDeletePresets() {
    // 从 URL 路径中提取名称: /api/preset/xxx
    String uri = server.uri();
    String name = "";
    int lastSlash = uri.lastIndexOf('/');
    if (lastSlash >= 0 && lastSlash < (int)uri.length() - 1) {
        name = uri.substring(lastSlash + 1);
    }
    // 也兼容 ?name=xxx 查询参数
    if (name.length() == 0 && server.hasArg("name")) {
        name = server.arg("name");
    }
    if (name.length() == 0) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"name required\"}");
        return;
    }

    Preferences prefs;
    prefs.begin("presets", false);
    uint8_t cnt = prefs.getUChar("cnt", 0);

    int foundIdx = -1;
    for (uint8_t i = 0; i < cnt && i < 20; i++) {
        if (prefs.getString(("p" + String(i) + "_nm").c_str(), "") == name) {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx < 0) {
        prefs.end();
        server.send(404, "application/json", "{\"ok\":false,\"error\":\"not found\"}");
        return;
    }

    // 前移覆盖
    for (int i = foundIdx; i < cnt - 1; i++) {
        String src = "p" + String(i + 1) + "_";
        String dst = "p" + String(i) + "_";
        prefs.putString((dst + "nm").c_str(), prefs.getString((src + "nm").c_str(), ""));
        prefs.putString((dst + "js").c_str(), prefs.getString((src + "js").c_str(), ""));
    }

    String last = "p" + String(cnt - 1) + "_";
    prefs.remove((last + "nm").c_str());
    prefs.remove((last + "js").c_str());
    prefs.putUChar("cnt", cnt - 1);
    prefs.end();

    server.send(200, "application/json", "{\"ok\":true}");
    Serial.printf("[PRESET] Deleted '%s' (idx=%d)\n", name.c_str(), foundIdx);
}

static void handlePostCommand() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"no body\"}");
    return;
  }

  String body = server.arg("plain");
  Serial.printf("[HTTP-CMD] Received (%d bytes)\n", body.length());

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("[HTTP-CMD] JSON error: %s\n", err.c_str());
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid json\"}");
    return;
  }

  extern void executeCommand(JsonDocument &doc);
  executeCommand(doc);

  server.send(200, "application/json", "{\"ok\":true}");
  Serial.println("[HTTP-CMD] Executed");
}



static void handlePostConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  String body = server.arg("plain");
  Serial.printf("[WEB] PostConfig (%d bytes)\n", body.length());

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("[WEB] JSON error: %s\n", err.c_str());
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  bool apHiddenChanged = false;
  bool needReconnect = false;

  if (doc.containsKey("staSsid")) {
    String newSsid = doc["staSsid"].as<String>();
    if (newSsid.length() > 0) {
      config.staSsid = newSsid;
      needReconnect = true;
    }
  }
  if (doc.containsKey("staPass")) {
    config.staPass = doc["staPass"].as<String>();
  }
  if (doc.containsKey("apHidden")) {
    bool nv = doc["apHidden"].as<bool>();
    if (nv != config.apHidden) {
      config.apHidden = nv;
      apHiddenChanged = true;
    }
  }
  if (doc.containsKey("mqttHost")) {
    String newHost = doc["mqttHost"].as<String>();
    if (newHost.length() > 0) {
      config.mqttHost = newHost;
    }
  }
  if (doc.containsKey("mqttPort")) {
    config.mqttPort = doc["mqttPort"].as<uint16_t>();
  }
  if (doc.containsKey("mqttUser")) {
    config.mqttUser = doc["mqttUser"].as<String>();
  }
  if (doc.containsKey("mqttPass")) {
    config.mqttPass = doc["mqttPass"].as<String>();
  }
  if (doc.containsKey("mqttClientId")) {
    config.mqttClientId = doc["mqttClientId"].as<String>();
  }
  if (doc.containsKey("mqttSsl")) {
    config.mqttSsl = doc["mqttSsl"].as<bool>();
  }
  if (doc.containsKey("mqttEnabled")) {                  // ← 加这一段
    config.mqttEnabled = doc["mqttEnabled"].as<bool>();  //
  }
  if (doc.containsKey("deviceName")) {
    config.deviceName = doc["deviceName"].as<String>();
  }
  if (doc.containsKey("batchInterval")) {
    config.batchInterval = doc["batchInterval"].as<int>();
    Serial.printf("[WEB] Batch interval: %d s\n", config.batchInterval);
  }

  if (doc.containsKey("subTopics")) {
    config.subTopics.clear();
    JsonArray arr = doc["subTopics"].as<JsonArray>();
    for (JsonObject o : arr) {
      TopicEntry t;
      t.topic = o["topic"].as<String>();
      t.qos = o["qos"] | 0;
      if (t.topic.length() > 0) {
        if (t.topic == MQTT_OTA_TOPIC) {
          Serial.printf("[WEB] REJECTED sub topic: %s (reserved)\n", t.topic.c_str());
          continue;
        }
        config.subTopics.push_back(t);
      }
    }
    Serial.printf("[WEB] SubTopics: %d\n", config.subTopics.size());
  }

  if (doc.containsKey("pubTopics")) {
    config.pubTopics.clear();
    JsonArray arr = doc["pubTopics"].as<JsonArray>();
    for (JsonObject o : arr) {
      TopicEntry t;
      t.topic = o["topic"].as<String>();
      t.qos = o["qos"] | 0;
      if (t.topic.length() > 0) {
        if (t.topic == MQTT_OTA_TOPIC) {
          Serial.printf("[WEB] REJECTED pub topic: %s (reserved)\n", t.topic.c_str());
          continue;
        }
        config.pubTopics.push_back(t);
      }
    }
    Serial.printf("[WEB] PubTopics: %d\n", config.pubTopics.size());
  }

  config.save();
  server.send(200, "application/json", "{\"ok\":true}");
  Serial.println("[WEB] Config saved");

  if (apHiddenChanged) {
    WifiManager::restartAP();
  }

  if (needReconnect) {
    WiFi.disconnect();
    if (config.staSsid.length() > 0) {
      WiFi.begin(config.staSsid.c_str(), config.staPass.c_str());
    }
  }

  MqttClient::disconnect();
}

static void handleScanWifi() {
  int n = WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (int i = 0; i < n; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["ssid"] = WiFi.SSID(i);
    o["rssi"] = WiFi.RSSI(i);
  }
  WiFi.scanDelete();
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

static void handleGetInfo() {
  JsonDocument doc;
  doc["apIP"] = WifiManager::getApIP();
  doc["staIP"] = WifiManager::getStaIP();
  doc["mqttStatus"] = MqttClient::isConnected() ? "Connected" : "Disconnected";
  doc["heapFree"] = ESP.getFreeHeap();
  doc["firmware"] = FIRMWARE_VERSION;
  doc["mac"] = WiFi.macAddress();
   doc["deviceName"] = config.deviceName; 
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

static void handleGetDevice() {
  JsonDocument doc;
  doc["deviceId"] = getDeviceId();
  doc["deviceName"] = config.deviceName;
  doc["ssid"] = getUniqueApSsid();
  doc["apIP"] = WifiManager::getApIP();
  doc["staIP"] = WifiManager::getStaIP();
  doc["firmware"] = FIRMWARE_VERSION;
  doc["mqttHost"] = config.mqttHost;
  doc["mqttStatus"] = MqttClient::isConnected() ? "connected" : "disconnected";
  doc["heapFree"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["chipModel"] = ESP.getChipModel();
  doc["chipRev"] = ESP.getChipRevision();
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["batchInterval"] = config.batchInterval;

  JsonArray snrs = doc["sensors"].to<JsonArray>();
  for (auto &s : config.sensors) {
    JsonObject o = snrs.add<JsonObject>();
    o["pin"] = s.pin;
    o["interval"] = s.interval;
    o["type"] = s.type;
    o["enabled"] = s.enabled;
    o["label"] = s.label;
  }

  JsonArray ins = doc["inputs"].to<JsonArray>();
  for (auto &t : config.inputs) {
    JsonObject o = ins.add<JsonObject>();
    o["pin"] = t.pin;
    o["mode"] = t.mode;
    o["enabled"] = t.enabled;
    o["label"] = t.label;
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ==================== 新增：设备名称独立保存 ====================

static void handlePostDevice() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  String body = server.arg("plain");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  if (doc.containsKey("deviceName")) {
    config.deviceName = doc["deviceName"].as<String>();
    Serial.printf("[WEB] Device name updated: %s\n", config.deviceName.c_str());
  }

  config.save();
  server.send(200, "application/json", "{\"ok\":true}");
}


static void handleGetPinCapabilities() {
  JsonDocument doc;
  doc["type"] = "pin_capabilities";
  doc["deviceId"] = getDeviceId();
  JsonArray arr = doc["pins"].to<JsonArray>();
  for (int i = 0; i < pinCapsCount; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["pin"] = pinCaps[i].pin;
    o["digitalIO"] = pinCaps[i].digitalIO;
    o["pwm"] = pinCaps[i].pwm;
    o["adc"] = pinCaps[i].adc;
    if (pinCaps[i].adc && pinCaps[i].adcChannel != 0xFF) {
      o["adcChannel"] = pinCaps[i].adcChannel;
    }
    o["touch"] = pinCaps[i].touch;
    o["altFunc"] = pinCaps[i].altFunc;
  }
  // 添加禁止引脚说明
  doc["note"] = "GPIO 33-37 为 Flash SPI 专用，禁止使用";
  doc["validPins"] = validExternalPinsCount;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}



// ==================== 新增：批量状态查询 ====================

static void handleGetBatchStatus() {
  JsonDocument doc;
  doc["type"] = "batch_status";
  doc["deviceId"] = getDeviceId();
   doc["mac"] = WiFi.macAddress();//设备芯片序列号
  doc["timestamp"] = millis();
  doc["heapFree"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["otaVerify"] = OTA_VERIFY_TAG;  // 新增

  JsonArray snrs = doc["sensors"].to<JsonArray>();
  for (auto &s : config.sensors) {
    if (!s.enabled) continue;
    JsonObject o = snrs.add<JsonObject>();
    o["pin"] = s.pin;
    o["label"] = s.label;
    o["raw"] = analogRead(s.pin);
  }

  JsonArray ins = doc["inputs"].to<JsonArray>();
  for (auto &t : config.inputs) {
    if (!t.enabled) continue;
    JsonObject o = ins.add<JsonObject>();
    o["pin"] = t.pin;
    o["label"] = t.label;
    o["value"] = digitalRead(t.pin);
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ==================== 系统监控页面 ====================

static void handleMonitorPage() {
  server.send_P(200, "text/html", MONITOR_PAGE);
}

static void handleGetSystem() {
  JsonDocument doc;

  // 内存
  doc["heapSize"] = ESP.getHeapSize();
  doc["heapFree"] = ESP.getFreeHeap();
  doc["heapMinFree"] = ESP.getMinFreeHeap();
  doc["heapMaxAlloc"] = ESP.getMaxAllocHeap();
  doc["psramSize"] = ESP.getPsramSize();
  doc["psramFree"] = ESP.getFreePsram();
  doc["psramMinFree"] = ESP.getMinFreePsram();

  // 存储
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["sketchSize"] = ESP.getSketchSize();
  doc["sketchFree"] = ESP.getFreeSketchSpace();

  // CPU
  doc["cpuFreq"] = ESP.getCpuFreqMHz();
  doc["chipModel"] = ESP.getChipModel();
  doc["chipRev"] = ESP.getChipRevision();

  // 网络
  doc["rssi"] = WiFi.RSSI();
  doc["staIP"] = WifiManager::getStaIP();
  doc["apIP"] = WifiManager::getApIP();
  doc["mqttStatus"] = MqttClient::isConnected() ? "connected" : "disconnected";

  // 系统
  doc["uptime"] = millis() / 1000;
  doc["firmware"] = FIRMWARE_VERSION;
  doc["deviceId"] = getDeviceId();
  doc["deviceName"] = config.deviceName;

  // 任务统计
  int at = 0;
  for (auto &t : TimerEngine::getList())
    if (t.enabled) at++;
  doc["activeTimers"] = at;
  doc["totalTimers"] = TimerEngine::getList().size();

  int ar = 0;
  for (auto &r : LogicEngine::getList())
    if (r.enabled) ar++;
  doc["activeRules"] = ar;
  doc["totalRules"] = LogicEngine::getList().size();

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}


// ==================== 指令编辑器页面 ====================

static void handleCmdBuilderPage() {
  server.send_P(200, "text/html", CMD_BUILDER_PAGE);
}


static void handleGetDevices() {
  JsonDocument doc;
  doc["deviceId"] = getDeviceId();
  doc["deviceName"] = config.deviceName;
  JsonArray arr = doc["devices"].to<JsonArray>();

  // 自己
  JsonObject self = arr.add<JsonObject>();
  self["id"]   = getDeviceId();
  self["name"] = config.deviceName;
  self["ip"]   = WifiManager::getApIP();
  self["lan"]  = true;

  // DualChannel 注册的设备
  for (int i = 0; i < DualChannel::getDeviceCount(); i++) {
    DeviceEntry *dev = DualChannel::getDeviceByIndex(i);
    if (!dev) continue;
    JsonObject o = arr.add<JsonObject>();
    o["id"]   = dev->deviceId;
    o["name"] = dev->deviceName;
    o["ip"]   = dev->ip;
    o["lan"]  = dev->lanOnline;
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}


// ==================== OTA ====================


static void handleOtaPage() {
  server.send_P(200, "text/html", OTA_PAGE);
}

static void wsOtaEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[OTA-WS] Client #%u disconnected\n", num);
      if (otaInProgress) {
        Update.abort();
        otaInProgress = false;
      }
      break;

    case WStype_CONNECTED:
      Serial.printf("[OTA-WS] Client #%u connected\n", num);
      // 重置 OTA 状态，防止被之前的连接卡住
      if (otaInProgress) {
        Serial.println("[OTA-WS] Reset stale OTA state");
        Update.abort();
        otaInProgress = false;
      }
      wsOta.sendTXT(num, "CONNECTED");
      break;


    case WStype_BIN:
      if (!otaInProgress || !Update.isRunning()) break;
      if (Update.write(payload, length) != length) {
        Serial.println("[OTA-WS] Write FAILED");
        Update.printError(Serial);
        wsOta.sendTXT(num, "ERR:Write failed");
        Update.abort();
        otaInProgress = false;
      }
      break;

    case WStype_TEXT:
      {
        String msg = String((char *)payload);

        // OTA: START
        if (msg.startsWith("START")) {
          if (otaInProgress) {
            wsOta.sendTXT(num, "ERR:Another OTA in progress");
            break;
          }
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Serial.println("[OTA-WS] Begin FAILED");
            Update.printError(Serial);
            wsOta.sendTXT(num, "ERR:Update.begin failed");
          } else {
            otaInProgress = true;
            Serial.println("[OTA-WS] Begin OK");
            wsOta.sendTXT(num, "READY");
          }
          break;
        }

        // OTA: DONE
        if (msg == "DONE") {
          if (!otaInProgress || !Update.isRunning()) {
            Serial.println("[OTA-WS] DONE but not active");
            break;
          }
          Serial.println("[OTA-WS] Finalizing...");
          if (Update.end(true)) {
            Serial.println("[OTA-WS] SUCCESS");
            otaInProgress = false;
            wsOta.sendTXT(num, "OK");
            delay(500);
            ESP.restart();
          } else {
            Serial.println("[OTA-WS] End FAILED");
            Update.printError(Serial);
            otaInProgress = false;
            wsOta.sendTXT(num, "ERR:Update.end failed");
          }
          break;
        }

        // 通用 JSON 指令通道
             // 通用 JSON 指令通道
      if (!otaInProgress && msg.startsWith("{")) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, msg);
        if (!err) {
          extern void executeCommand(JsonDocument &doc);
          extern void setResultSink(uint8_t, void(*)(uint8_t, const char*));
          setResultSink(num, wsResultForward);
          executeCommand(doc);
          setResultSink(0xFF, nullptr);
          Serial.printf("[WS-CMD] Executed from client #%u\n", num);
          wsOta.sendTXT(num, "{\"type\":\"ws_ack\",\"status\":\"ok\"}");
        } else {
          Serial.printf("[WS-CMD] JSON error: %s\n", err.c_str());
          wsOta.sendTXT(num, "{\"type\":\"ws_ack\",\"status\":\"error\"}");
        }
      }
      break;  // ← 这个 break 原本就有，不用动
    }


        default:
          break;
      }
  }


  // ==================== WebServer 初始化 ====================

  namespace WebServerManager {

  void init() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/config", HTTP_POST, handlePostConfig);
    server.on("/api/scan", HTTP_GET, handleScanWifi);
    server.on("/api/info", HTTP_GET, handleGetInfo);
    server.on("/api/device", HTTP_GET, handleGetDevice);
    server.on("/api/device", HTTP_POST, handlePostDevice);
    server.on("/ota", HTTP_GET, handleOtaPage);

    server.on("/api/pins", HTTP_GET, handleGetPinCapabilities);
    server.on("/api/batch_status", HTTP_GET, handleGetBatchStatus);
    server.on("/pins", HTTP_GET, handlePinsPage);
    server.on("/monitor", HTTP_GET, handleMonitorPage);
    server.on("/api/system", HTTP_GET, handleGetSystem);
    server.on("/builder", HTTP_GET, handleCmdBuilderPage);
  
   server.on("/api/devices", HTTP_GET, handleGetDevices);

server.on("/api/presets", HTTP_GET, handleGetPresets);
server.on("/api/preset", HTTP_POST, handlePostPresets);
server.on("/api/preset", HTTP_DELETE, handleDeletePresets);
server.on("/api/command", HTTP_POST, handlePostCommand);

   

    server.on("/api/pins", HTTP_GET, handleGetPinCapabilities);


    server.begin();

    wsOta.begin();
    wsOta.onEvent(wsOtaEvent);
    Serial.println("[WEB] Server on 80, OTA-WS on 8080");
  }

  void loop() {
    server.handleClient();
    wsOta.loop();
  }

  }  // namespace WebServerManager
