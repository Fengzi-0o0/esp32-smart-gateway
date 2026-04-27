#ifndef RAND_ENGINE_H
#define RAND_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace Rand {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);

    int   getInt(int minVal, int maxVal);
    float getFloat(float minVal, float maxVal);
    bool  getBool();
}

#endif
