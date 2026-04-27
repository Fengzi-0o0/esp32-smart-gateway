#include "dual_channel.h"
#include "config.h"
#include "mqtt_client.h"
#include <WiFi.h>
#include <WiFiUDP.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>

static DeviceEntry devices[DC_MAX_DEVICES];
static int devCount = 0;
static PendingMsg pending[DC_PENDING_SIZE];
static unsigned long lastDiscover = 0;
static int lanConsecutiveFails = 0;
static unsigned long lastLanFailTime = 0;


// ===== LAN WebSocket 客户端 =====
static WebSocketsClient wsClient;
static bool wsBusy = false;
static unsigned long wsStart = 0;
static String lastLanIp = "";
static uint16_t lastLanPort = 0;
static unsigned long lastLanOk = 0;

namespace LanClient {

bool sendToDevice(const String &ip, uint16_t port, const String &json) {
    if (wsBusy) return false;
    wsBusy = true;
    wsStart = millis();

    // 同一目标且连接在2秒内，复用
    bool reuse = (ip == lastLanIp && port == lastLanPort &&
                  wsClient.isConnected() && (millis() - lastLanOk < 2000));

    if (!reuse) {
        wsClient.disconnect();
        wsClient.begin(ip, port, "/");

        unsigned long t0 = millis();
        while (millis() - t0 < 300) {
            wsClient.loop();
            if (wsClient.isConnected()) break;
            yield();
        }
        lastLanIp = ip;
        lastLanPort = port;
    }

    bool ok = false;
    if (wsClient.isConnected()) {
        String payload = json;
        ok = wsClient.sendTXT(payload);
        lastLanOk = millis();
        Serial.printf("[LAN] %s %d bytes to %s:%u\n",
                      ok ? "Sent" : "FAIL", json.length(), ip.c_str(), port);
    } else {
        Serial.printf("[LAN] Connect timeout %s:%u\n", ip.c_str(), port);
        lastLanIp = "";
    }

    wsBusy = false;
    return ok;
}


void loop() {
    // 发送超时清理
    if (wsBusy && millis() - wsStart > 3000) {
        wsClient.disconnect();
        wsBusy = false;
        lastLanIp = "";
        Serial.println("[LAN] Client timeout, cleaned up");
    }
    // 空闲超过10秒断开连接释放资源
     if (!wsBusy && lastLanIp.length() > 0 && (millis() - lastLanOk > 10000)) {
        wsClient.disconnect();
        lastLanIp = "";
    }
}

} // namespace LanClient


// ===== 内部工具 =====

static bool mqttPublish(const String &json) {
    if (!MqttClient::isConnected()) return false;
    if (config.pubTopics.size() == 0) return false;
    MqttClient::publish(config.pubTopics[0].topic.c_str(), json.c_str());
    return true;
}

static void registerPendingMsg(const String &target, const String &json, uint16_t mid) {
    if (mid == 0) return;
    for (int i = 0; i < DC_PENDING_SIZE; i++) {
        if (!pending[i].active) {
            pending[i].mid = mid;
            pending[i].target = target;
            pending[i].json = json;
            pending[i].retries = 0;
            pending[i].sentTime = millis();
            pending[i].active = true;
            return;
        }
    }
}

static uint16_t extractMid(const String &json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) return 0;
    if (!doc.containsKey("_mid")) return 0;
    String s = doc["_mid"].as<String>();
    if (s.length() == 0) return 0;
    return (uint16_t)strtol(s.c_str(), NULL, 16);
}

// ===== 命名空间实现 =====

namespace DualChannel {

void init() {
    devCount = 0;
    memset(pending, 0, sizeof(pending));
    lastDiscover = 0;
    Serial.println("[DC] Dual channel initialized");
}

void loop() {
    unsigned long now = millis();

    // 定时 UDP 发现
    if (WiFi.isConnected() && now - lastDiscover > DC_DISCOVER_MS) {
        lastDiscover = now;
        discoverDevices();
    }

    // LAN 客户端维护
    LanClient::loop();

    // 超时重试

    for (int i = 0; i < DC_PENDING_SIZE; i++) {
        if (!pending[i].active) continue;
        if (now - pending[i].sentTime < DC_ACK_TIMEOUT) continue;
        if (pending[i].retries < DC_MAX_RETRIES) {
            pending[i].retries++;
            pending[i].sentTime = now;
            sendToTarget(pending[i].target, pending[i].json);
            Serial.printf("[DC] Retry %d _mid=0x%04X\n",
                          pending[i].retries, pending[i].mid);
        } else {
            pending[i].active = false;
            Serial.printf("[DC] Gave up _mid=0x%04X\n", pending[i].mid);
        }
    }

    // 设备超时标记
    for (int i = 0; i < devCount; i++) {
        if (devices[i].lanOnline && (now - devices[i].lastSeen > DC_DEVICE_TIMEOUT)) {
            devices[i].lanOnline = false;
            Serial.printf("[DC] %s LAN timeout\n", devices[i].deviceName.c_str());
        }
    }
}

// ===== 设备注册表 =====

void registerDevice(const String &id, const String &name,
                    const String &ip, uint16_t port) {
    for (int i = 0; i < devCount; i++) {
        if (devices[i].deviceId == id) {
            devices[i].deviceName = name;
            devices[i].ip = ip;
            devices[i].port = port;
            devices[i].lastSeen = millis();
            devices[i].lanOnline = true;
            return;
        }
    }
    if (devCount < DC_MAX_DEVICES) {
        DeviceEntry &d = devices[devCount++];
        d.deviceId = id;
        d.deviceName = name;
        d.ip = ip;
        d.port = port;
        d.lastSeen = millis();
        d.lanOnline = true;
        d.mqttOnline = false;
        Serial.printf("[DC] Registered: %s (%s) @ %s:%u\n",
                      id.c_str(), name.c_str(), ip.c_str(), port);
    }
}

void updateMqttStatus(const String &id, bool online) {
    for (int i = 0; i < devCount; i++) {
        if (devices[i].deviceId == id) {
            devices[i].mqttOnline = online;
            return;
        }
    }
    // 不在注册表中 → 自动注册（无 IP）
    if (online && devCount < DC_MAX_DEVICES) {
        DeviceEntry &d = devices[devCount++];
        d.deviceId = id;
        d.deviceName = "";
        d.ip = "";
        d.port = 0;
        d.lastSeen = millis();
        d.lanOnline = false;
        d.mqttOnline = true;
        Serial.printf("[DC] MQTT-only: %s\n", id.c_str());
    }
}

DeviceEntry* findDevice(const String &query) {
    for (int i = 0; i < devCount; i++) {
        if (devices[i].deviceId == query) return &devices[i];
        if (devices[i].deviceName.length() > 0 &&
            devices[i].deviceName == query) return &devices[i];
    }
    return nullptr;
}

int getDeviceCount() {
    return devCount;
}

// ===== 路由决策 =====

RouteChannel decideRoute(DeviceEntry *dev) {
    if (!dev) {
        if (MqttClient::isConnected()) return ROUTE_MQTT;
        return ROUTE_NONE;
    }
    bool lanOk = dev->lanOnline && dev->ip.length() > 0 &&
                 (millis() - dev->lastSeen < DC_DEVICE_TIMEOUT);
    if (lanOk) return ROUTE_LAN;
    if (MqttClient::isConnected()) return ROUTE_MQTT;  // 不要求 mqttOnline
    return ROUTE_NONE;
}




// ===== 统一发送 =====

bool sendToTarget(const String &target, const String &json) {
    DeviceEntry *dev = findDevice(target);
    RouteChannel route = decideRoute(dev);

    bool ok = false;
    switch (route) {
        case ROUTE_LAN:
            // LAN 最近失败过且在冷却期内，直接走 MQTT
            if (millis() - lastLanFailTime < 10000 && MqttClient::isConnected()) {
                ok = MqttClient::publish(json);
                Serial.printf("[DC] LAN cooldown, MQTT→ %s\n", target.c_str());
                break;
            }
            ok = LanClient::sendToDevice(dev->ip, dev->port ? dev->port : 8080, json);
            Serial.printf("[DC] LAN→ %s (%s)\n", target.c_str(), ok ? "OK" : "FAIL");
            if (!ok) {
                lastLanFailTime = millis();
                if (MqttClient::isConnected()) {
                    ok = MqttClient::publish(json);
                    Serial.printf("[DC] Fallback MQTT→ %s\n", target.c_str());
                }
            }
            break;


            ok = LanClient::sendToDevice(dev->ip, dev->port ? dev->port : 8080, json);
            Serial.printf("[DC] LAN→ %s (%s)\n", target.c_str(), ok ? "OK" : "FAIL");
            if (!ok) {
                dev->lanOnline = false;  // 标记离线，下次不再尝试LAN
                dev->lastSeen = 0;
                Serial.printf("[DC] Marked %s LAN offline\n", target.c_str());
                if (MqttClient::isConnected()) {
                    ok = MqttClient::publish(json);
                    Serial.printf("[DC] Fallback MQTT→ %s\n", target.c_str());
                }
            }
            break;

        case ROUTE_MQTT:
            ok = MqttClient::publish(json);
            Serial.printf("[DC] MQTT→ %s\n", target.c_str());
            break;
        case ROUTE_NONE:
            Serial.printf("[DC] OFFLINE: %s\n", target.c_str());
            break;
    }

    if (ok) {
        uint16_t mid = extractMid(json);
        registerPendingMsg(target, json, mid);
    }
    return ok;
}

bool sendToTargetLan(const String &target, const String &json) {
    DeviceEntry *dev = findDevice(target);
    if (!dev) return false;

    bool lanOk = dev->lanOnline && dev->ip.length() > 0 &&
                 (millis() - dev->lastSeen < DC_DEVICE_TIMEOUT);
    if (!lanOk) return false;

    bool ok = LanClient::sendToDevice(dev->ip, dev->port ? dev->port : 8080, json);
    Serial.printf("[DC] LAN-only→ %s (%s)\n", target.c_str(), ok ? "OK" : "FAIL");
    return ok;
}

bool broadcastAll(const String &json) {
    bool anySent = false;
    for (int i = 0; i < devCount; i++) {
        RouteChannel route = decideRoute(&devices[i]);
        if (route == ROUTE_LAN) {
            if (LanClient::sendToDevice(devices[i].ip,
                    devices[i].port ? devices[i].port : 8080, json))
                anySent = true;
        } else if (route == ROUTE_MQTT) {
            if (MqttClient::publish(json)) anySent = true;
        }
    }
    // 兜底：发一份无 target 的 MQTT
    if (MqttClient::publish(json)) anySent = true;
    return anySent;
}


// ===== ACK =====

void handleAck(uint16_t mid) {
    for (int i = 0; i < DC_PENDING_SIZE; i++) {
        if (pending[i].active && pending[i].mid == mid) {
            pending[i].active = false;
            Serial.printf("[DC] ACK 0x%04X confirmed\n", mid);
            return;
        }
    }
}

// ===== UDP 发现 =====

void discoverDevices() {
    if (!WiFi.isConnected()) return;

    JsonDocument req;
    req["action"] = "discover";
    req["deviceId"] = getDeviceId();
    req["deviceName"] = config.deviceName;
    String json;
    serializeJson(req, json);

    WiFiUDP udp;
    udp.beginPacket(IPAddress(255, 255, 255, 255), UDP_DISCOVERY_PORT);
    udp.write((const uint8_t *)json.c_str(), json.length());
    udp.endPacket();
    udp.stop();
}


// ===== 状态查询 =====

String toJson() {
    JsonDocument doc;
    doc["type"] = "dual_channel";
    doc["count"] = devCount;
    JsonArray arr = doc["devices"].to<JsonArray>();
    for (int i = 0; i < devCount; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["id"] = devices[i].deviceId;
        o["name"] = devices[i].deviceName;
        o["ip"] = devices[i].ip;
        o["lan"] = devices[i].lanOnline;
        o["mqtt"] = devices[i].mqttOnline;
        RouteChannel r = decideRoute(&devices[i]);
        o["route"] = (r == ROUTE_LAN) ? "lan" :
                     (r == ROUTE_MQTT) ? "mqtt" : "none";
    }
    String json;
    serializeJson(doc, json);
    return json;
}
DeviceEntry* getDeviceByIndex(int idx) {
    if (idx < 0 || idx >= devCount) return nullptr;
    return &devices[idx];
}
} // namespace DualChannel
