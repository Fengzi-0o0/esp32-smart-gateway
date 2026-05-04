#ifndef DISPLAY_ENGINE_H
#define DISPLAY_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace DisplayEngine {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);

    // I2C 冲突检测用
    extern int displaySda;
    extern int displayScl;
}

#endif
