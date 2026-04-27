#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>

namespace MqttClient {
    void init();
    void loop();
    void disconnect();
    bool isConnected();
    bool publish(const String &json);
    bool publish(const String &topic, const String &payload, uint8_t qos = 0);
    void publish(const char* topic, const char* payload);
    void publish(const char* topic, const uint8_t* payload, unsigned int length);
}

#endif
