#ifndef RTC_ENGINE_H
#define RTC_ENGINE_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

#define MAX_RTC_SCHEDULES 10

struct RtcSchedule {
    String id;
    int year, month, day, hour, minute, second;
    bool repeat;       // true=每天重复, false=单次
    bool enabled;
    String cron;  
    bool fired;        // 单次任务是否已执行
    String commandsJson;
};

namespace RTC {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);

    bool   syncNtp();
    bool   isTimeValid();
    String getTimeString();
    String getDateString();
    unsigned long getEpoch();

    void addSchedule(const RtcSchedule &s);
    bool removeSchedule(const String &id);
    void enableSchedule(const String &id, bool en);
    String schedulesToJson();
}

#endif
