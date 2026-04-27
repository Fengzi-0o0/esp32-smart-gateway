#ifndef TOUCH_ENGINE_H
#define TOUCH_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace Touch {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);
}

#endif
