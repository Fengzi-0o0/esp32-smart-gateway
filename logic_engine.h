#ifndef LOGIC_ENGINE_H
#define LOGIC_ENGINE_H

#include <Arduino.h>
#include <vector>

#define MAX_LOGIC_RULES 10

struct SimpleCondition {
    String source;  // "sensor" 或 "input"
    int    pin;
    String op;      // ">", "<", "=", "!=" (sensor); "high", "low" (input)
    float  value;   // 阈值 (sensor)
    String source_var;  // 新增：当 source=="var" 时使用
};

struct LogicRule {
    String id;
    String operator_;              // "and", "or"（仅顶层）
    std::vector<SimpleCondition> conditions;
    String actionsJson;            // JSON array of action commands
    bool enabled;
    bool prevCondition;            // 上一次条件结果（边沿检测）
    unsigned long lastTrigger;     // 上次触发时间
    unsigned long cooldown;        // 冷却时间 ms
};

namespace LogicEngine {
    void init();
    void loop();
    void add(const LogicRule &r);
    bool remove(const String &id);
    void enable(const String &id, bool en);
    std::vector<LogicRule>& getList();
    String toJson();
}

#endif
