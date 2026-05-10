#ifndef DUAL_CHANNEL_H
#define DUAL_CHANNEL_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define DC_MAX_DEVICES      10
#define DC_PENDING_SIZE      4
#define DC_ACK_TIMEOUT    2000
#define DC_MAX_RETRIES       3
#define DC_DISCOVER_MS   30000
#define DC_DEVICE_TIMEOUT 120000
#define DC_MQTT_HEARTBEAT_MS 60000
#define DC_MAX_HOPS          2

struct DeviceEntry {
    String deviceId;
    String deviceName;
    String ip;
    uint16_t port;
    unsigned long lastLanSeen;
    unsigned long lastMqttSeen;
    bool lanOnline;
    bool mqttOnline;
    String via;
};

struct PendingMsg {
    uint16_t mid;
    String target;
    String json;
    uint8_t retries;
    unsigned long sentTime;
    bool active;
};

enum RouteChannel { ROUTE_LAN, ROUTE_MQTT, ROUTE_RELAY, ROUTE_NONE };

// ===== LAN WebSocket 客户端（纯传输层） =====
namespace LanClient {
    bool sendToDevice(const String &ip, uint16_t port, const String &json);
    void loop();
}

namespace DualChannel {
    void init();
    void loop();

    void registerDevice(const String &id, const String &name,
                        const String &ip, uint16_t port);
    void registerRemoteDevice(const String &id, const String &name,
                              const String &viaId);
    void updateMqttStatus(const String &id, bool online, const String &name = "");
    DeviceEntry* findDevice(const String &query);
    int  getDeviceCount();

    RouteChannel decideRoute(DeviceEntry *dev);
    bool sendToTarget(const String &target, const String &json);
    bool sendToTargetLan(const String &target, const String &json);
    bool broadcastAll(const String &json);

    void handleAck(uint16_t mid);
    void discoverDevices();
    String toJson();
    DeviceEntry* getDeviceByIndex(int idx);
}

#endif
