#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "mqtt_client.h"
#include "dual_channel.h"  


static bool staConnected = false;
static WiFiUDP udp;
static bool udpStarted = false;

// 新增: STA 连接/断开事件回调函数指针（供批量上报等模块监听）
typedef void (*StaEventCallback)(bool connected);
static StaEventCallback staEventCb = nullptr;

static void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            staConnected = true;
            MqttClient::publishLog("info", "WIFI", "STA Connected - IP: %s", WiFi.localIP().toString().c_str());
            WifiManager::startUdpDiscovery();
            if (staEventCb) staEventCb(true);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            staConnected = false;
            udpStarted = false;
            MqttClient::publishLog("warn", "WIFI", "STA Disconnected, will retry...");
            if (staEventCb) staEventCb(false);
            break;
        default:
            break;
    }
}

namespace WifiManager {

void init() {
    WiFi.onEvent(onWiFiEvent);
    WiFi.mode(WIFI_AP_STA);

    String apSsid = getUniqueApSsid();
    WiFi.softAP(apSsid.c_str(), DEFAULT_AP_PASS, 1, config.apHidden ? 1 : 0);

    Serial.printf("[WIFI] AP Started - SSID: %s - IP: %s - Hidden: %s\n",
                  apSsid.c_str(),
                  WiFi.softAPIP().toString().c_str(),
                  config.apHidden ? "YES" : "NO");

    if (config.staSsid.length() > 0) {
        Serial.printf("[WIFI] Connecting to: %s\n", config.staSsid.c_str());
        WiFi.begin(config.staSsid.c_str(), config.staPass.c_str());
    } else {
        Serial.println("[WIFI] No STA credentials configured");
    }
}

void loop() {
    static unsigned long lastRetry = 0;
    if (!staConnected && config.staSsid.length() > 0) {
        if (millis() - lastRetry > 30000) {
            lastRetry = millis();
            Serial.println("[WIFI] Retrying STA connection...");
            WiFi.disconnect();
            WiFi.begin(config.staSsid.c_str(), config.staPass.c_str());
        }
    }
    handleUdpDiscovery();
}

void restartAP() {
    WiFi.softAPdisconnect(true);
    delay(200);
    String apSsid = getUniqueApSsid();
    WiFi.softAP(apSsid.c_str(), DEFAULT_AP_PASS, 1, config.apHidden ? 1 : 0);
    Serial.printf("[WIFI] AP Restarted - SSID: %s - Hidden: %s - IP: %s\n",
                  apSsid.c_str(),
                  config.apHidden ? "YES" : "NO",
                  WiFi.softAPIP().toString().c_str());
}

bool isStaConnected() {
    return staConnected;
}

String getStaIP() {
    return staConnected ? WiFi.localIP().toString() : "N/A";
}

String getApIP() {
    return WiFi.softAPIP().toString();
}

void startUdpDiscovery() {
    if (udpStarted) return;
    udp.begin(UDP_DISCOVERY_PORT);
    udpStarted = true;
    Serial.printf("[UDP] Discovery listening on port %d\n", UDP_DISCOVERY_PORT);
}

void handleUdpDiscovery() {
    if (!udpStarted) return;

    int packetSize = udp.parsePacket();
    if (packetSize == 0) return;

    char buf[512];
    int len = udp.read(buf, sizeof(buf) - 1);
    if (len <= 0) return;
    buf[len] = '\0';

    JsonDocument req;
    DeserializationError err = deserializeJson(req, buf);
    if (err) return;

       String action = req["action"].as<String>();

    // 收到 discover_response → 注册对方设备
    if (action == "discover_response") {
        String senderId = req["deviceId"] | String("");
        String senderName = req["deviceName"] | String("");
        if (senderId.length() > 0 && senderId != getDeviceId()) {
            DualChannel::registerDevice(senderId, senderName,
                                         udp.remoteIP().toString(), 8080);
            Serial.printf("[UDP] Registered from response: %s (%s) @ %s\n",
                          senderId.c_str(), senderName.c_str(),
                          udp.remoteIP().toString().c_str());
            if (req.containsKey("remoteDevices")) {
                for (JsonObject rd : req["remoteDevices"].as<JsonArray>()) {
                    String rId = rd["id"] | String("");
                    String rName = rd["name"] | String("");
                    DualChannel::registerRemoteDevice(rId, rName, senderId);
                }
            }
        }
        return;
    }

    if (action != "discover") return;


    Serial.printf("[UDP] Discovery from %s:%d\n",
                  udp.remoteIP().toString().c_str(), udp.remotePort());

    // 发送响应（保留原有）
    JsonDocument resp;
    resp["action"]        = "discover_response";
    resp["deviceId"]      = getDeviceId();
    resp["deviceName"]    = config.deviceName;
    resp["ssid"]          = getUniqueApSsid();
    resp["apPass"]        = DEFAULT_AP_PASS;
    resp["apIP"]          = getApIP();
    resp["staIP"]         = getStaIP();
    resp["firmware"]      = FIRMWARE_VERSION;
    resp["mqtt"]          = MqttClient::isConnected() ? "connected" : "disconnected";
    resp["heap"]          = ESP.getFreeHeap();
    resp["uptime"]        = millis() / 1000;
    resp["batchInterval"] = config.batchInterval;
    resp["mac"]           = WiFi.macAddress();

    JsonArray remotes = resp["remoteDevices"].to<JsonArray>();
    for (int i = 0; i < DualChannel::getDeviceCount(); i++) {
        DeviceEntry *dev = DualChannel::getDeviceByIndex(i);
        if (!dev) continue;
        if (!dev->mqttOnline) continue;
        if (dev->lanOnline) continue;
        if (dev->via.length() > 0) continue;
        JsonObject ro = remotes.add<JsonObject>();
        ro["id"] = dev->deviceId;
        ro["name"] = dev->deviceName;
    }

    String response;
    serializeJson(resp, response);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write((const uint8_t*)response.c_str(), response.length());
    udp.endPacket();

    // 新增：注册发送方为设备
    String senderId = "";
    String senderName = "";
    if (req.containsKey("deviceId")) senderId = req["deviceId"].as<String>();
    if (req.containsKey("deviceName")) senderName = req["deviceName"].as<String>();

    if (senderId.length() > 0 && senderId != getDeviceId()) {
        DualChannel::registerDevice(senderId, senderName,
                                     udp.remoteIP().toString(), 8080);
    }

    Serial.printf("[UDP] Response sent\n");
}


} // namespace WifiManager
