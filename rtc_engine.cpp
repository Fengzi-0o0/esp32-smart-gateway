#include "rtc_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include <time.h>
#include <Preferences.h>
#include <WiFi.h>

static const char* NTP1 = "ntp.aliyun.com";
static const char* NTP2 = "pool.ntp.org";

static unsigned long bootEpoch  = 0;
static bool          timeValid  = false;
static unsigned long lastSync   = 0;
static const unsigned long SYNC_INTERVAL = 3600000;

static std::vector<RtcSchedule> schedules;
static Preferences rtcPrefs;

// ================================================================
//  时间工具
// ================================================================

static unsigned long currentEpoch() {
    if (!timeValid) return 0;
    return bootEpoch + (millis() / 1000);
}

static void epochToTm(unsigned long epoch, struct tm &t) {
    time_t tt = (time_t)epoch;
    localtime_r(&tt, &t);
}

// ================================================================
//  Cron 解析器
//  格式: "分 时 日 月 周"
//  支持: *  /  -  ,  具体数值
//  周: 0=周日 1=周一 ... 6=周六
// ================================================================

struct CronFields {
    bool valid;
    String minute, hour, dayOfMonth, month, dayOfWeek;
};

static bool matchCronField(const String &field, int value, int minVal, int maxVal) {
    if (field == "*") return true;

    int commaPos = 0;
    while (commaPos < (int)field.length()) {
        int nextComma = field.indexOf(',', commaPos);
        String part;
        if (nextComma < 0) {
            part = field.substring(commaPos);
            commaPos = field.length();
        } else {
            part = field.substring(commaPos, nextComma);
            commaPos = nextComma + 1;
        }

        int step = 1;
        int slashPos = part.indexOf('/');
        String rangePart;
        if (slashPos >= 0) {
            rangePart = part.substring(0, slashPos);
            step = part.substring(slashPos + 1).toInt();
            if (step <= 0) step = 1;
        } else {
            rangePart = part;
        }

        int low, high;
        int dashPos = rangePart.indexOf('-');
        if (dashPos >= 0) {
            low = rangePart.substring(0, dashPos).toInt();
            high = rangePart.substring(dashPos + 1).toInt();
        } else if (rangePart == "*") {
            low = minVal;
            high = maxVal;
        } else {
            low = high = rangePart.toInt();
        }

        if (value >= low && value <= high && (value - low) % step == 0) {
            return true;
        }
    }
    return false;
}

static CronFields parseCron(const String &expr) {
    CronFields cf;
    cf.valid = false;

    String parts[5];
    int start = 0;
    for (int i = 0; i < 5; i++) {
        int sp = expr.indexOf(' ', start);
        if (sp < 0 && i < 4) return cf;
        if (sp < 0) sp = expr.length();
        parts[i] = expr.substring(start, sp);
        parts[i].trim();
        if (parts[i].length() == 0) return cf;
        start = sp + 1;
    }

    cf.minute     = parts[0];
    cf.hour       = parts[1];
    cf.dayOfMonth = parts[2];
    cf.month      = parts[3];
    cf.dayOfWeek  = parts[4];
    cf.valid = true;
    return cf;
}

static bool isCronMatch(const CronFields &cf, const struct tm &t) {
    if (!matchCronField(cf.minute,     t.tm_min,  0, 59)) return false;
    if (!matchCronField(cf.hour,       t.tm_hour, 0, 23)) return false;
    if (!matchCronField(cf.dayOfMonth, t.tm_mday, 1, 31)) return false;
    if (!matchCronField(cf.month,      t.tm_mon + 1, 1, 12)) return false;
    if (!matchCronField(cf.dayOfWeek,  t.tm_wday, 0, 6))  return false;
    return true;
}

// ================================================================
//  调度判断（cron + 传统时间）
// ================================================================

static bool isScheduleDue(const RtcSchedule &s, unsigned long now) {
    struct tm ct;
    epochToTm(now, ct);

    // cron 模式
    if (s.cron.length() > 0) {
        CronFields cf = parseCron(s.cron);
        if (!cf.valid) return false;
        return isCronMatch(cf, ct);
    }

    // 传统模式
    return (ct.tm_year + 1900 == s.year || s.repeat) &&
           (ct.tm_mon + 1    == s.month || s.repeat) &&
           (ct.tm_mday       == s.day   || s.repeat) &&
            ct.tm_hour == s.hour &&
            ct.tm_min  == s.minute &&
            ct.tm_sec  == s.second;
}

// ================================================================
//  NVS 持久化
// ================================================================

static void loadSchedules() {
    rtcPrefs.begin("rtc", true);
    uint8_t cnt = rtcPrefs.getUChar("scnt", 0);
    schedules.clear();
    for (uint8_t i = 0; i < cnt && i < MAX_RTC_SCHEDULES; i++) {
        RtcSchedule s;
        String p = "s" + String(i) + "_";
        s.id        = rtcPrefs.getString((p + "id").c_str(), "");
        s.cron      = rtcPrefs.getString((p + "cron").c_str(), "");
        s.year      = rtcPrefs.getInt((p + "y").c_str(), 2025);
        s.month     = rtcPrefs.getInt((p + "m").c_str(), 1);
        s.day       = rtcPrefs.getInt((p + "d").c_str(), 1);
        s.hour      = rtcPrefs.getInt((p + "h").c_str(), 0);
        s.minute    = rtcPrefs.getInt((p + "mi").c_str(), 0);
        s.second    = rtcPrefs.getInt((p + "s").c_str(), 0);
        s.repeat    = rtcPrefs.getBool((p + "r").c_str(), false);
        s.enabled   = rtcPrefs.getBool((p + "e").c_str(), true);
        s.fired     = rtcPrefs.getBool((p + "f").c_str(), false);
        s.commandsJson = rtcPrefs.getString((p + "c").c_str(), "[]");
        if (s.id.length() > 0) schedules.push_back(s);
    }
    rtcPrefs.end();
    Serial.printf("[RTC] Loaded %d schedules\n", schedules.size());
}

static void saveSchedules() {
    rtcPrefs.begin("rtc", false);
    rtcPrefs.putUChar("scnt", schedules.size());
    for (uint8_t i = 0; i < schedules.size() && i < MAX_RTC_SCHEDULES; i++) {
        String p = "s" + String(i) + "_";
        rtcPrefs.putString((p + "id").c_str(),   schedules[i].id);
        rtcPrefs.putString((p + "cron").c_str(), schedules[i].cron);
        rtcPrefs.putInt((p + "y").c_str(),       schedules[i].year);
        rtcPrefs.putInt((p + "m").c_str(),       schedules[i].month);
        rtcPrefs.putInt((p + "d").c_str(),       schedules[i].day);
        rtcPrefs.putInt((p + "h").c_str(),       schedules[i].hour);
        rtcPrefs.putInt((p + "mi").c_str(),      schedules[i].minute);
        rtcPrefs.putInt((p + "s").c_str(),       schedules[i].second);
        rtcPrefs.putBool((p + "r").c_str(),      schedules[i].repeat);
        rtcPrefs.putBool((p + "e").c_str(),      schedules[i].enabled);
        rtcPrefs.putBool((p + "f").c_str(),      schedules[i].fired);
        rtcPrefs.putString((p + "c").c_str(),    schedules[i].commandsJson);
    }
    rtcPrefs.end();
    Serial.println("[RTC] Schedules saved");
}

// ================================================================
//  引用外部命令执行
// ================================================================

extern void executeCommand(JsonDocument &doc);

// ================================================================
//  RTC 命名空间
// ================================================================

namespace RTC {

void init() {
    configTime(8 * 3600, 0, NTP1, NTP2);
    loadSchedules();
    Serial.println("[RTC] Engine initialized, NTP syncing...");
}

void loop() {
    if (millis() - lastSync > SYNC_INTERVAL) {
        lastSync = millis();
        syncNtp();
    }

    if (!timeValid) return;
    unsigned long now = currentEpoch();

    for (auto &s : schedules) {
        if (!s.enabled) continue;
        if (!s.repeat && s.fired) continue;

        if (isScheduleDue(s, now)) {
            if (s.repeat) {
                static unsigned long lastFire[MAX_RTC_SCHEDULES] = {0};
                int idx = &s - &schedules[0];
                if (now - lastFire[idx] < 2) continue;
                lastFire[idx] = now;
            } else {
                s.fired = true;
                saveSchedules();
            }

            Serial.printf("[RTC] Schedule '%s' fired at %s\n",
                          s.id.c_str(), getTimeString().c_str());

            JsonDocument actDoc;
            if (!deserializeJson(actDoc, s.commandsJson)) {
                if (actDoc.is<JsonArray>()) {
                    for (JsonObject a : actDoc.as<JsonArray>()) {
                        JsonDocument tmp;
                        tmp.set(a);
                        executeCommand(tmp);
                    }
                } else if (actDoc.is<JsonObject>()) {
                    executeCommand(actDoc);
                }
            }
        }
    }
}

bool syncNtp() {
    configTime(8 * 3600, 0, NTP1, NTP2);
    struct tm t;
    if (getLocalTime(&t, 5000)) {
        time_t now = mktime(&t);
        bootEpoch  = now - (millis() / 1000);
        timeValid  = true;
        Serial.printf("[RTC] NTP sync OK: %04d-%02d-%02d %02d:%02d:%02d\n",
                      t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                      t.tm_hour, t.tm_min, t.tm_sec);
        return true;
    }
    Serial.println("[RTC] NTP sync FAILED");
    return false;
}

bool isTimeValid() { return timeValid; }
unsigned long getEpoch() { return currentEpoch(); }

String getTimeString() {
    if (!timeValid) return "N/A";
    struct tm t;
    epochToTm(currentEpoch(), t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    return String(buf);
}

String getDateString() {
    if (!timeValid) return "N/A";
    struct tm t;
    epochToTm(currentEpoch(), t);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    return String(buf);
}

void addSchedule(const RtcSchedule &s) {
    for (auto &e : schedules) {
        if (e.id == s.id) { e = s; saveSchedules(); return; }
    }
    if (schedules.size() < MAX_RTC_SCHEDULES) {
        schedules.push_back(s);
        saveSchedules();
    }
}

bool removeSchedule(const String &id) {
    for (auto it = schedules.begin(); it != schedules.end(); ++it) {
        if (it->id == id) { schedules.erase(it); saveSchedules(); return true; }
    }
    return false;
}

void enableSchedule(const String &id, bool en) {
    for (auto &s : schedules) {
        if (s.id == id) {
            s.enabled = en;
            if (en) s.fired = false;
            saveSchedules();
            return;
        }
    }
}

String schedulesToJson() {
    JsonDocument doc;
    doc["type"] = "rtc_schedules";
    doc["timeValid"] = timeValid;
    doc["currentTime"] = getTimeString();
    doc["currentDate"] = getDateString();
    doc["epoch"] = currentEpoch();
    JsonArray arr = doc["schedules"].to<JsonArray>();
    for (auto &s : schedules) {
        JsonObject o = arr.add<JsonObject>();
        o["id"] = s.id;
        if (s.cron.length() > 0) {
            o["cron"] = s.cron;
        } else {
            char dt[20];
            snprintf(dt, sizeof(dt), "%04d-%02d-%02d %02d:%02d:%02d",
                     s.year, s.month, s.day, s.hour, s.minute, s.second);
            o["datetime"] = dt;
        }
        o["repeat"]  = s.repeat;
        o["enabled"] = s.enabled;
        o["fired"]   = s.fired;
        JsonDocument cmds;
        if (!deserializeJson(cmds, s.commandsJson)) o["commands"] = cmds;
    }
    String json;
    serializeJson(doc, json);
    return json;
}

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();

    if (action == "sync") {
        bool ok = syncNtp();
        JsonDocument resp;
        resp["type"]     = "rtc_sync";
        resp["deviceId"] = getDeviceId();
        resp["success"]  = ok;
        resp["time"]     = getTimeString();
        resp["date"]     = getDateString();
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "get_time") {
        JsonDocument resp;
        resp["type"]     = "rtc_time";
        resp["deviceId"] = getDeviceId();
        resp["valid"]    = timeValid;
        resp["time"]     = getTimeString();
        resp["date"]     = getDateString();
        resp["epoch"]    = currentEpoch();
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "set_time") {
        int y  = doc["year"]   | 2025;
        int mo = doc["month"]  | 1;
        int d  = doc["day"]    | 1;
        int h  = doc["hour"]   | 0;
        int mi = doc["minute"] | 0;
        int s  = doc["second"] | 0;
        struct tm t = {};
        t.tm_year = y - 1900;
        t.tm_mon  = mo - 1;
        t.tm_mday = d;
        t.tm_hour = h;
        t.tm_min  = mi;
        t.tm_sec  = s;
        time_t epoch = mktime(&t);
        bootEpoch = epoch - (millis() / 1000);
        timeValid = true;
        Serial.printf("[RTC] Manual set: %s %s\n", getDateString().c_str(), getTimeString().c_str());

    } else if (action == "add_schedule") {
        RtcSchedule s;
        s.id      = doc["id"].as<String>();
        s.cron    = doc["cron"] | String("");
        s.year    = doc["year"]   | 2025;
        s.month   = doc["month"]  | 1;
        s.day     = doc["day"]    | 1;
        s.hour    = doc["hour"]   | 0;
        s.minute  = doc["minute"] | 0;
        s.second  = doc["second"] | 0;
        s.repeat  = doc["repeat"] | false;
        s.enabled = doc["enabled"] | true;
        s.fired   = false;

        if (s.cron.length() > 0) {
            CronFields cf = parseCron(s.cron);
            if (!cf.valid) {
                Serial.printf("[RTC] Invalid cron: %s\n", s.cron.c_str());
                return;
            }
            s.repeat = true;  // cron 自动为循环模式
            Serial.printf("[RTC] Cron parsed OK: %s\n", s.cron.c_str());
        }

        if (doc.containsKey("commands")) {
            String cs;
            serializeJson(doc["commands"], cs);
            s.commandsJson = cs;
        } else {
            s.commandsJson = "[]";
        }
        addSchedule(s);
        Serial.printf("[RTC] Schedule added: %s\n", s.id.c_str());

    } else if (action == "remove_schedule") {
        String id = doc["id"].as<String>();
        removeSchedule(id);

    } else if (action == "enable_schedule") {
        String id = doc["id"].as<String>();
        bool en   = doc["enabled"] | true;
        enableSchedule(id, en);

    } else if (action == "list_schedules") {
        String json = schedulesToJson();
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, json);

    } else {
        Serial.printf("[RTC] Unknown action: %s\n", action.c_str());
    }
}

} // namespace RTC
