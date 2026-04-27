#ifndef SPI_ENGINE_H
#define SPI_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace SPIBus {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);
}

#endif
