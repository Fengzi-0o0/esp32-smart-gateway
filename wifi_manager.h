#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

namespace WifiManager {
    void init();
    void loop();
    void restartAP();
    bool isStaConnected();
    String getStaIP();
    String getApIP();
    void startUdpDiscovery();
    void handleUdpDiscovery();
}

#endif
