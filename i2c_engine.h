#ifndef I2C_ENGINE_H
#define I2C_ENGINE_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

namespace I2C {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);
    bool scanDevices(int sda, int scl, std::vector<uint8_t> &found);
}

#endif
