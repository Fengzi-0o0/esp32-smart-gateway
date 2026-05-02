#ifndef ENCODER_ENGINE_H
#define ENCODER_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace Encoder {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);
}

#endif
