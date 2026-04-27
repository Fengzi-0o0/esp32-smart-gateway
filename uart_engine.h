#ifndef UART_ENGINE_H
#define UART_ENGINE_H

#include <Arduino.h>
#include <ArduinoJson.h>

namespace UART {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);
    uint32_t buildConfig(int dataBits, int parity, int stopBits);
}

#endif
