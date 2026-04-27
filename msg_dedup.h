#ifndef MSG_DEDUP_H
#define MSG_DEDUP_H

#include <Arduino.h>

#define DEDUP_CACHE_SIZE 16

class MsgDedup {
public:
    void init() {
        memset(_cache, 0, sizeof(_cache));
        _idx = 0;
    }

    bool isDuplicate(uint16_t mid) {
        if (mid == 0) return false;
        for (int i = 0; i < DEDUP_CACHE_SIZE; i++) {
            if (_cache[i] == mid) return true;
        }
        return false;
    }

    void record(uint16_t mid) {
        if (mid == 0) return;
        _cache[_idx] = mid;
        _idx = (_idx + 1) % DEDUP_CACHE_SIZE;
    }

    // 从 JsonDocument 安全提取 _mid，不存在返回 0
    static uint16_t parseMid(JsonDocument &doc) {
        if (!doc.containsKey("_mid")) return 0;
        String s = doc["_mid"].as<String>();
        if (s.length() == 0) return 0;
        return (uint16_t)strtol(s.c_str(), NULL, 16);
    }

private:
    uint16_t _cache[DEDUP_CACHE_SIZE];
    uint8_t  _idx;
};

#endif
