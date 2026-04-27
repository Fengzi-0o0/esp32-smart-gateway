#ifndef SCRIPT_ENGINE_H
#define SCRIPT_ENGINE_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

struct Variable {
    String name;
    float value;
    bool persistent;
};

struct SequenceState {
    String id;
    String commandsJson;
    int step;
    int total;
    unsigned long lastStep;
    unsigned long stepDelay;
    bool active;
    String target;
};

struct LoopState {
    String id;
    String commandsJson;
    int count;
    int executed;
    unsigned long interval;
    unsigned long lastRun;
    bool active;
    String target;
};

namespace ScriptEngine {
    void init();
    void loop();
    void handleCommand(JsonDocument &doc);

    float  getVar(const String &name, float defaultVal = 0);
    void   setVar(const String &name, float value, bool persistent = false);
    bool   removeVar(const String &name);
    void   clearVars();
    float  incrementVar(const String &name, float step = 1.0f);

    void addSequence(const String &id, const String &cmdsJson,
                     unsigned long stepDelay, const String &target);
    void addLoop(const String &id, const String &cmdsJson,
                 int count, unsigned long interval, const String &target);

    String toJson();
}

#endif
