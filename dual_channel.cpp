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
static unsigned long lastMqttHeartbeat = 0;
static int lanConsecutiveFails = 0;
static unsigned long lastLanFailTime = 0;
// 放在 sendToTarget 函数之前
static int consecutiveLanFails = 0;
static unsigned long lastLanAttempt = 0;



// ===== LAN WebSocket 客户端 =====
static WebSocketsClient wsClient;
static bool wsBusy = false;
static unsigned long wsStart = 0;
static String lastLanIp = "";
static uint16_t lastLanPort = 0;
static unsigned long lastLanOk = 0;

static void wsClientEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[LAN-WS] Connected to server\n");
      break;
    case WStype_TEXT:
      // 收到设备B回发的数据（GPIO状态等），直接丢弃
      // 不处理就不会阻塞，但必须读出来清空TCP缓冲区
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[LAN-WS] Disconnected\n");
      lastLanIp = "";  // 清除缓存，下次重连
      break;
    case WStype_ERROR:
      Serial.printf("[LAN-WS] Error\n");
      lastLanIp = "";
      break;
    default:
      break;
  }
}

namespace LanClient {

bool sendToDevice(const String &ip, uint16_t port, const String &json) {
  if (wsBusy) {
    Serial.printf("[LAN] Busy, skip %s:%u\n", ip.c_str(), port);
    return false;
  }
  wsBusy = true;
  wsStart = millis();

  // 同一目标且连接在 5 秒内，复用（★ 从2秒改为5秒）
  bool reuse = (ip == lastLanIp && port == lastLanPort && wsClient.isConnected() && (millis() - lastLanOk < 5000));

  if (reuse) {
    // ★ 新增：复用前调用 loop() 处理回发数据，保持连接活跃
    wsClient.loop();
    // 再次检查连接是否仍然存活
    if (!wsClient.isConnected()) {
      reuse = false;
      lastLanIp = "";
    }
  }

  if (!reuse) {
    wsClient.disconnect();
    delay(10);
    wsClient.onEvent(wsClientEvent);  // ★ 新增：注册事件处理
    wsClient.begin(ip, port, "/");
    wsClient.setReconnectInterval(100);  // ★ 新增：断线后100ms自动重连

    unsigned long t0 = millis();
    while (millis() - t0 < 500) {  // ★ 从300ms改为500ms
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
    if (ok) {
      lastLanOk = millis();
      // ★ 新增：发送后立即 loop() 处理回发数据
      wsClient.loop();
    }
    Serial.printf("[LAN] %s %d bytes to %s:%u\n",
                  ok ? "Sent" : "FAIL", json.length(), ip.c_str(), port);
  } else {
    Serial.printf("[LAN] Connect timeout %s:%u\n", ip.c_str(), port);
    wsClient.disconnect();
    lastLanIp = "";
  }

  wsBusy = false;
  return ok;
}

void loop() {
  // ★ 新增：空闲期间也调用 loop() 维持连接
  if (!wsBusy && lastLanIp.length() > 0 && wsClient.isConnected()) {
    wsClient.loop();
  }

  // 发送超时清理
  if (wsBusy && millis() - wsStart > 3000) {
    wsClient.disconnect();
    wsBusy = false;
    lastLanIp = "";
    Serial.println("[LAN] Client timeout, cleaned up");
  }
  // 空闲超过 30 秒断开连接释放资源（★ 从10秒改为30秒）
  if (!wsBusy && lastLanIp.length() > 0 && (millis() - lastLanOk > 30000)) {
    wsClient.disconnect();
    lastLanIp = "";
  }
}

}  // namespace LanClient



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

  // 定时 MQTT 心跳
  if (MqttClient::isConnected() && now - lastMqttHeartbeat > DC_MQTT_HEARTBEAT_MS) {
    lastMqttHeartbeat = now;
    JsonDocument hb;
    hb["deviceId"] = getDeviceId();
    hb["deviceName"] = config.deviceName;
    hb["online"] = true;
    String json;
    serializeJson(hb, json);
    MqttClient::publish("esp32/status", json.c_str());
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

  // LAN 设备超时标记
  for (int i = 0; i < devCount; i++) {
    if (devices[i].lanOnline && (now - devices[i].lastLanSeen > DC_DEVICE_TIMEOUT)) {
      devices[i].lanOnline = false;
      MqttClient::publishLog("warn", "DC", "%s LAN timeout", devices[i].deviceName.c_str());
    }
  }

  // MQTT 设备超时标记
  for (int i = 0; i < devCount; i++) {
    if (devices[i].mqttOnline && (now - devices[i].lastMqttSeen > DC_DEVICE_TIMEOUT)) {
      devices[i].mqttOnline = false;
      MqttClient::publishLog("warn", "DC", "%s MQTT timeout", devices[i].deviceName.c_str());
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
      devices[i].lastLanSeen = millis();
      devices[i].lanOnline = true;
      devices[i].via = "";
      return;
    }
  }
  if (devCount < DC_MAX_DEVICES) {
    DeviceEntry &d = devices[devCount++];
    d.deviceId = id;
    d.deviceName = name;
    d.ip = ip;
    d.port = port;
    d.lastLanSeen = millis();
    d.lastMqttSeen = 0;
    d.lanOnline = true;
    d.mqttOnline = false;
    d.via = "";
    Serial.printf("[DC] Registered: %s (%s) @ %s:%u\n",
                  id.c_str(), name.c_str(), ip.c_str(), port);
  }
}

void registerRemoteDevice(const String &id, const String &name,
                          const String &viaId) {
  if (id.length() == 0 || id == getDeviceId()) return;
  for (int i = 0; i < devCount; i++) {
    if (devices[i].deviceId == id) {
      if (devices[i].lanOnline || devices[i].mqttOnline) return;
      devices[i].deviceName = name;
      devices[i].via = viaId;
      Serial.printf("[DC] Remote updated: %s (%s) via %s\n",
                    id.c_str(), name.c_str(), viaId.c_str());
      return;
    }
  }
  if (devCount < DC_MAX_DEVICES) {
    DeviceEntry &d = devices[devCount++];
    d.deviceId = id;
    d.deviceName = name;
    d.ip = "";
    d.port = 0;
    d.lastLanSeen = 0;
    d.lastMqttSeen = 0;
    d.lanOnline = false;
    d.mqttOnline = false;
    d.via = viaId;
    MqttClient::publishLog("info", "DC", "Remote: %s (%s) via %s",
                  id.c_str(), name.c_str(), viaId.c_str());
  }
}

void updateMqttStatus(const String &id, bool online, const String &name) {
  for (int i = 0; i < devCount; i++) {
    if (devices[i].deviceId == id) {
      devices[i].mqttOnline = online;
      if (online) devices[i].lastMqttSeen = millis();
      if (name.length() > 0) devices[i].deviceName = name;
      return;
    }
  }
  if (online && devCount < DC_MAX_DEVICES) {
    DeviceEntry &d = devices[devCount++];
    d.deviceId = id;
    d.deviceName = name;
    d.ip = "";
    d.port = 0;
    d.lastLanSeen = 0;
    d.lastMqttSeen = millis();
    d.lanOnline = false;
    d.mqttOnline = true;
    Serial.printf("[DC] MQTT-only: %s (%s)\n", id.c_str(), name.c_str());
  }
}

DeviceEntry *findDevice(const String &query) {
  for (int i = 0; i < devCount; i++) {
    if (devices[i].deviceId == query) return &devices[i];
    if (devices[i].deviceName.length() > 0 && devices[i].deviceName == query) return &devices[i];
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
  bool lanOk = dev->lanOnline && dev->ip.length() > 0 && (millis() - dev->lastLanSeen < DC_DEVICE_TIMEOUT);
  if (lanOk) return ROUTE_LAN;
  if (dev->mqttOnline && MqttClient::isConnected()) return ROUTE_MQTT;
  if (dev->via.length() > 0) {
    DeviceEntry *relay = findDevice(dev->via);
    if (relay) {
      bool relayLan = relay->lanOnline && relay->ip.length() > 0 && (millis() - relay->lastLanSeen < DC_DEVICE_TIMEOUT);
      bool relayMqtt = relay->mqttOnline && MqttClient::isConnected();
      if (relayLan || relayMqtt) return ROUTE_RELAY;
    }
  }
  if (MqttClient::isConnected()) return ROUTE_MQTT;
  return ROUTE_NONE;
}





// ===== 统一发送 =====

bool sendToTarget(const String &target, const String &json) {
  DeviceEntry *dev = findDevice(target);
  RouteChannel route = decideRoute(dev);

  bool ok = false;
  switch (route) {
    case ROUTE_LAN:
      {
        // —— 冷却窗口：连续失败越多，等越久再尝试 LAN ——
        unsigned long cooldownMs = 0;
        if (consecutiveLanFails > 0) {
          cooldownMs = 10000UL * (1UL << (consecutiveLanFails - 1));
          if (cooldownMs > 40000UL) cooldownMs = 40000UL;
        }

        if (consecutiveLanFails > 0 && millis() - lastLanAttempt < cooldownMs) {
          // 冷却期内，跳过 LAN，直接走 MQTT
          if (MqttClient::isConnected()) {
            ok = MqttClient::publish(json);
            Serial.printf("[DC] LAN cooldown (%d fails, %lus), MQTT→ %s\n",
                          consecutiveLanFails, cooldownMs / 1000, target.c_str());
          } else {
            Serial.printf("[DC] LAN cooldown + MQTT offline: %s\n", target.c_str());
          }
          break;
        }

        // —— 冷却结束，尝试 LAN ——
        lastLanAttempt = millis();
        ok = LanClient::sendToDevice(dev->ip, dev->port ? dev->port : 8080, json);
        MqttClient::publishLog("info", "DC", "LAN→ %s (%s)", target.c_str(), ok ? "OK" : "FAIL");

        if (ok) {
          // 成功：重置失败计数，LAN 优先恢复
          consecutiveLanFails = 0;
        } else {
          // 失败：累加计数，进入冷却
          consecutiveLanFails++;
          Serial.printf("[DC] LAN fail #%d, next retry in %lus\n",
                        consecutiveLanFails,
                        (10000UL * (1UL << (consecutiveLanFails - 1))) / 1000);

          // 回退 MQTT
          if (MqttClient::isConnected()) {
            ok = MqttClient::publish(json);
            MqttClient::publishLog("info", "DC", "Fallback MQTT→ %s", target.c_str());
          }
        }
        break;
      }
    case ROUTE_MQTT:
      ok = MqttClient::publish(json);
      MqttClient::publishLog("info", "DC", "MQTT→ %s", target.c_str());
      break;
    case ROUTE_RELAY:
      {
        DeviceEntry *relay = findDevice(dev->via);
        if (!relay) {
          MqttClient::publishLog("warn", "DC", "Relay %s not found for %s", dev->via.c_str(), target.c_str());
          break;
        }
        bool relayLan = relay->lanOnline && relay->ip.length() > 0 && (millis() - relay->lastLanSeen < DC_DEVICE_TIMEOUT);
        if (relayLan) {
          ok = LanClient::sendToDevice(relay->ip, relay->port ? relay->port : 8080, json);
          MqttClient::publishLog("info", "DC", "RELAY LAN→ %s via %s (%s)",
                        target.c_str(), dev->via.c_str(), ok ? "OK" : "FAIL");
          if (!ok && MqttClient::isConnected()) {
            ok = MqttClient::publish(json);
            MqttClient::publishLog("info", "DC", "RELAY fallback MQTT→ %s via %s", target.c_str(), dev->via.c_str());
          }
        } else if (relay->mqttOnline && MqttClient::isConnected()) {
          ok = MqttClient::publish(json);
          MqttClient::publishLog("info", "DC", "RELAY MQTT→ %s via %s (%s)",
                        target.c_str(), dev->via.c_str(), ok ? "OK" : "FAIL");
        } else {
          MqttClient::publishLog("warn", "DC", "RELAY OFFLINE: %s (relay %s down)", target.c_str(), dev->via.c_str());
        }
        break;
      }
    case ROUTE_NONE:
      MqttClient::publishLog("warn", "DC", "OFFLINE: %s", target.c_str());
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

  bool lanOk = dev->lanOnline && dev->ip.length() > 0 && (millis() - dev->lastLanSeen < DC_DEVICE_TIMEOUT);
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
    } else if (route == ROUTE_MQTT || route == ROUTE_RELAY) {
      if (MqttClient::publish(json)) anySent = true;
    }
  }
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
    o["lanAge"] = devices[i].lastLanSeen ? (int)((millis() - devices[i].lastLanSeen) / 1000) : -1;
    o["mqttAge"] = devices[i].lastMqttSeen ? (int)((millis() - devices[i].lastMqttSeen) / 1000) : -1;
    if (devices[i].via.length() > 0) o["via"] = devices[i].via;
    RouteChannel r = decideRoute(&devices[i]);
    o["route"] = (r == ROUTE_LAN) ? "lan" : (r == ROUTE_MQTT) ? "mqtt"
                  : (r == ROUTE_RELAY) ? "relay" : "none";
  }
  String json;
  serializeJson(doc, json);
  return json;
}
DeviceEntry *getDeviceByIndex(int idx) {
  if (idx < 0 || idx >= devCount) return nullptr;
  return &devices[idx];
}
}  // namespace DualChannel
