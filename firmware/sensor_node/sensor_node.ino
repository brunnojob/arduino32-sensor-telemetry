#include <Arduino.h>
#include <DHT.h>
#include "wire.h"
#ifndef DHT_PIN
#define DHT_PIN 4
#endif
#ifndef PIR_PIN
#define PIR_PIN 27
#endif
#ifndef DHT_TYPE
#define DHT_TYPE DHT11
#endif
DHT sensor(DHT_PIN, DHT_TYPE);
uint32_t sequence = 0;
uint32_t sampledAt = 0;
uint32_t presenceChangedAt = 0;
bool candidatePresence = false;
bool stablePresence = false;
void setup() {
    Serial.begin(115200);
    pinMode(PIR_PIN, INPUT);
    sensor.begin();
}
void loop() {
    const uint32_t now = millis();
    const bool presence = digitalRead(PIR_PIN) == HIGH;
    if (presence != candidatePresence) { candidatePresence = presence; presenceChangedAt = now; }
    if ((uint32_t)(now - presenceChangedAt) >= 100) stablePresence = candidatePresence;
    if ((uint32_t)(now - sampledAt) < 2000) { delay(1); return; }
    sampledAt = now;
    const float temperature = sensor.readTemperature();
    const float humidity = sensor.readHumidity();
    const bool valid = isfinite(temperature) && isfinite(humidity) && temperature >= -40 && temperature <= 80 && humidity >= 0 && humidity <= 100;
    const wire_sample sample{sequence++, now, (int16_t)(valid ? lroundf(temperature * 100) : 0), (uint16_t)(valid ? lroundf(humidity * 100) : 0), (uint8_t)stablePresence, (uint8_t)valid};
    uint8_t frame[WIRE_FRAME_SIZE];
    wire_encode(&sample, frame);
    Serial.write(frame, sizeof frame);
}
