#ifndef ONEWIRE_ENGINE_H
#define ONEWIRE_ENGINE_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

namespace OneWire {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);
}

#endif
