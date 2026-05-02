#include "gpio_control.h"
#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

struct PinState {
  int pin;
  int mode;
  int value;
  bool active;
};

#define MAX_PINS 20
static PinState pins[MAX_PINS];
static int pinCount = 0;

static PinState* findOrAddPin(int pin) {
  for (int i = 0; i < pinCount; i++) {
    if (pins[i].pin == pin) return &pins[i];
  }
  if (pinCount < MAX_PINS) {
    pins[pinCount].pin = pin;
    pins[pinCount].mode = OUTPUT;
    pins[pinCount].value = 0;
    pins[pinCount].active = true;
    return &pins[pinCount++];
  }
  return nullptr;
}

namespace GpioControl {

void init() {
  pinCount = 0;
  Serial.println("[GPIO] Initialized");
}

void digitalSet(int pin, int value) {
  PinState* ps = findOrAddPin(pin);
  if (!ps) return;
  pinMode(pin, OUTPUT);
  ps->mode = OUTPUT;
  ps->value = value ? 1 : 0;
  digitalWrite(pin, ps->value);
}

void digitalToggle(int pin) {
  PinState* ps = findOrAddPin(pin);
  if (!ps) return;
  pinMode(pin, OUTPUT);
  ps->mode = OUTPUT;
  ps->value = !ps->value;
  digitalWrite(pin, ps->value);
}

void analogSet(int pin, int value) {
  PinState* ps = findOrAddPin(pin);
  if (!ps) return;
  pinMode(pin, OUTPUT);
  ps->mode = OUTPUT;
  ps->value = constrain(value, 0, 255);
  analogWrite(pin, ps->value);
}

void setMode(int pin, int mode) {
  PinState* ps = findOrAddPin(pin);
  if (!ps) return;
  ps->mode = mode;
  pinMode(pin, mode);
}

int getPinMode(int pin) {
  for (int i = 0; i < pinCount; i++) {
    if (pins[i].pin == pin && pins[i].active) {
      return pins[i].mode;
    }
  }
  return -1;
}

void publishStatus() {
  if (!MqttClient::isConnected()) return;
  if (config.pubTopics.size() == 0) return;

  JsonDocument doc;
  doc["status"] = "pin_report";
  JsonArray arr = doc["pins"].to<JsonArray>();
  for (int i = 0; i < pinCount; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["pin"] = pins[i].pin;
    if (pins[i].mode == OUTPUT) {
      o["mode"] = "output";
    } else if (pins[i].mode == INPUT_PULLUP) {
      o["mode"] = "input_pullup";
    } else {
      o["mode"] = "input";
    }
    o["value"] = pins[i].value;
  }

  String payload;
  serializeJson(doc, payload);
  MqttClient::publish(config.pubTopics[0].topic, payload);
  Serial.println("[GPIO] Status published");
}

// ═══════════════════════════════════════════════════════════
// ★ 新增：脉冲测量
// ═══════════════════════════════════════════════════════════
long measurePulse(int pin, int trigPin, int level, int timeout, int samples) {
    pinMode(pin, INPUT);
    if (trigPin >= 0) {
        pinMode(trigPin, OUTPUT);
    }

    long readings[20];
    int validCount = 0;

    for (int s = 0; s < samples; s++) {
        if (trigPin >= 0) {
            digitalWrite(trigPin, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPin, LOW);
        }

        long duration = pulseIn(pin, level, timeout);
        if (duration > 0) {
            readings[validCount++] = duration;
        }
        if (s < samples - 1) delay(20);
    }

    long medianDuration = 0;
    if (validCount > 0) {
        for (int i = 0; i < validCount - 1; i++)
            for (int j = i + 1; j < validCount; j++)
                if (readings[j] < readings[i]) {
                    long tmp = readings[i];
                    readings[i] = readings[j];
                    readings[j] = tmp;
                }
        medianDuration = readings[validCount / 2];
    }

    return medianDuration;
}

// ═══════════════════════════════════════════════════════════
// ★ 新增：音频输出（方波频率）
// ═══════════════════════════════════════════════════════════
void toneSet(int pin, unsigned int freq) {
    if (freq == 0) {
        ledcDetach(pin);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    } else {
        ledcAttach(pin, freq, 8);
        ledcWriteTone(pin, freq);
    }
}

}  // namespace GpioControl
