#ifndef TIMER_ENGINE_H
#define TIMER_ENGINE_H

#include <Arduino.h>
#include <vector>

#define MAX_TIMERS 10

struct TimerTask {
    String id;
    String type;           // "interval", "once", "count"
    unsigned long interval; // ms
    int count;             // -1 = infinite
    int executed;
    bool enabled;
    unsigned long lastRun;
    String commandsJson;   // JSON array of commands
    unsigned long duration;    // 新增：运行时长限制(ms)，0=不限制
    unsigned long startTime;   // 新增：启动时间
    bool autoDelete;           // 新增：完成后自动删除
};

namespace TimerEngine {
    void init();
    void loop();
    void add(const TimerTask &t);
    bool remove(const String &id);
    void enable(const String &id, bool en);
    void reset(const String &id);
    std::vector<TimerTask>& getList();
    String toJson();
}

#endif
