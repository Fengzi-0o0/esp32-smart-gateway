#include "encoder_engine.h"
#include "config.h"
#include "mqtt_client.h"
#include "pin_caps.h"
#include "script_engine.h"
#include "driver/pcnt.h"

#define MAX_ENCODERS 4

struct EncoderState {
    String id;
    int clkPin;
    int dtPin;
    pcnt_unit_t unit;
    bool active;
    String label;
    long lastPosition;
};

static EncoderState encoders[MAX_ENCODERS];
static int encCount = 0;
static int nextUnit = 0;

static EncoderState* findEncoder(const String &id) {
    for (int i = 0; i < encCount; i++) {
        if (encoders[i].id == id && encoders[i].active) return &encoders[i];
    }
    return nullptr;
}

static long readPosition(EncoderState &enc) {
    int16_t count = 0;
    pcnt_get_counter_value(enc.unit, &count);
    return (long)count;
}

namespace Encoder {

void init() {
    encCount = 0;
    nextUnit = 0;
    Serial.println("[ENC] Engine initialized");
}

void loop() {
    // PCNT 硬件计数，无需轮询
}

void handleCommand(JsonDocument &doc) {
    String action = doc["action"].as<String>();

    if (action == "add") {
        String id = doc["id"].as<String>();
        int clk = doc["clk"] | -1;
        int dt  = doc["dt"]  | -1;
        String label = doc["label"] | String("");

        if (id.length() == 0) { Serial.println("[ENC] add: id required"); return; }
        if (clk < 0 || dt < 0) { Serial.println("[ENC] add: clk and dt required"); return; }
        if (!isValidExternalPin(clk)) { Serial.printf("[ENC] REJECTED: clk=%d\n", clk); return; }
        if (!isValidExternalPin(dt))  { Serial.printf("[ENC] REJECTED: dt=%d\n", dt); return; }

        // 已存在 → 先移除再重建
        for (int i = 0; i < encCount; i++) {
            if (encoders[i].id == id) {
                pcnt_counter_pause(encoders[i].unit);
                for (int j = i; j < encCount - 1; j++)
                    encoders[j] = encoders[j + 1];
                encCount--;
                break;
            }
        }

        if (nextUnit >= 4) { Serial.println("[ENC] Max 4 PCNT units"); return; }
        if (encCount >= MAX_ENCODERS) { Serial.println("[ENC] Max encoders reached"); return; }

        pcnt_unit_t unit = (pcnt_unit_t)nextUnit;

        pcnt_config_t cfg = {};
        cfg.pulse_gpio_num = clk;
        cfg.ctrl_gpio_num  = dt;
        cfg.channel        = PCNT_CHANNEL_0;
        cfg.unit           = unit;
        cfg.pos_mode       = PCNT_COUNT_INC;
        cfg.neg_mode       = PCNT_COUNT_DEC;
        cfg.lctrl_mode     = PCNT_MODE_REVERSE;
        cfg.hctrl_mode     = PCNT_MODE_KEEP;
        cfg.counter_h_lim  = 32767;
        cfg.counter_l_lim  = -32768;
        pcnt_unit_config(&cfg);

        pcnt_counter_pause(unit);
        pcnt_counter_clear(unit);
        pcnt_counter_resume(unit);

        EncoderState &enc = encoders[encCount];
        enc.id = id;
        enc.clkPin = clk;
        enc.dtPin  = dt;
        enc.unit   = unit;
        enc.active = true;
        enc.label  = label;
        enc.lastPosition = 0;
        encCount++;
        nextUnit++;

        Serial.printf("[ENC] Added '%s' CLK=%d DT=%d unit=%d\n",
                      id.c_str(), clk, dt, (int)unit);

        JsonDocument resp;
        resp["type"]     = "encoder_added";
        resp["deviceId"] = getDeviceId();
        resp["id"]       = id;
        resp["clk"]      = clk;
        resp["dt"]       = dt;
        resp["unit"]     = (int)unit;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "read") {
        String id = doc["id"].as<String>();
        EncoderState *enc = findEncoder(id);
        if (!enc) { Serial.printf("[ENC] '%s' not found\n", id.c_str()); return; }

        long pos   = readPosition(*enc);
        long delta = pos - enc->lastPosition;
        enc->lastPosition = pos;

        if (doc.containsKey("result_var")) {
            String varName = doc["result_var"].as<String>();
            if (varName.length() > 0)
                ScriptEngine::setVar(varName, (float)pos, false);
        }

        JsonDocument resp;
        resp["type"]     = "encoder";
        resp["deviceId"] = getDeviceId();
        resp["id"]       = id;
        resp["position"] = pos;
        resp["delta"]    = delta;
        resp["label"]    = enc->label;
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);

    } else if (action == "reset") {
        String id = doc["id"].as<String>();
        EncoderState *enc = findEncoder(id);
        if (!enc) { Serial.printf("[ENC] '%s' not found\n", id.c_str()); return; }

        pcnt_counter_clear(enc->unit);
        enc->lastPosition = 0;
        Serial.printf("[ENC] Reset '%s'\n", id.c_str());

    } else if (action == "remove") {
        String id = doc["id"].as<String>();
        for (int i = 0; i < encCount; i++) {
            if (encoders[i].id == id) {
                pcnt_counter_pause(encoders[i].unit);
                for (int j = i; j < encCount - 1; j++)
                    encoders[j] = encoders[j + 1];
                encCount--;
                Serial.printf("[ENC] Removed '%s'\n", id.c_str());
                break;
            }
        }

    } else if (action == "list") {
        JsonDocument resp;
        resp["type"]     = "encoder_list";
        resp["deviceId"] = getDeviceId();
        JsonArray arr = resp["encoders"].to<JsonArray>();
        for (int i = 0; i < encCount; i++) {
            JsonObject o = arr.add<JsonObject>();
            o["id"]       = encoders[i].id;
            o["clk"]      = encoders[i].clkPin;
            o["dt"]       = encoders[i].dtPin;
            o["unit"]     = (int)encoders[i].unit;
            o["position"] = readPosition(encoders[i]);
            o["active"]   = encoders[i].active;
            o["label"]    = encoders[i].label;
        }
        String p;
        serializeJson(resp, p);
        if (MqttClient::isConnected() && config.pubTopics.size() > 0)
            MqttClient::publish(config.pubTopics[0].topic, p);
        Serial.println("[ENC] List published");

    } else {
        Serial.printf("[ENC] Unknown action: %s\n", action.c_str());
    }
}

} // namespace Encoder
