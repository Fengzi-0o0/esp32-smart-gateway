#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <Arduino.h>

namespace GpioControl {
    void init();
    void digitalSet(int pin, int value);
    void digitalToggle(int pin);
    void analogSet(int pin, int value);
    void setMode(int pin, int mode);
    int  getPinMode(int pin);
    void publishStatus();
}

#endif
