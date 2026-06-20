/**
 * @file sketch.ino
 * @brief MineGuard — Event-driven prototype (no Arduino loop).
 *
 * Architecture:
 *   - loop() is EMPTY. No busy polling in the main thread.
 *   - Collision (KY-031) and SOS button are HARDWARE INTERRUPTS: the hardware
 *     notifies instantly, nothing is polled for them.
 *   - The four HC-SR04, the heart-rate chip and the GPS stream are sampled by a
 *     timer (Ticker) — a sensor that must be *asked* (ranging/analog/serial)
 *     cannot announce itself, so a 200 ms tick drives them. Each emits an event
 *     only when its value changes.
 *   - The LCD, LEDs and buzzer are refreshed ONLY when the state changes.
 *     Drag a distance slider and the display updates; idle = nothing redraws.
 *
 * Wokwi libraries: TinyGPSPlus, LiquidCrystal_I2C.
 */

#include "MineGuardDevice.h"
#include <Arduino.h>
#include <Wire.h>
#include <Ticker.h>

MineGuardDevice device;
Ticker samplingEngine;

// Timer-driven sampling tick (replaces the Arduino loop)
void onSamplingTick() {
    device.update();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22); // I2C: SDA=21, SCL=22

    Serial.println("MineGuard event-driven prototype starting...");
    device.initialize();

    // Drive sampling from a timer; buttons/collision already fire via interrupts.
    samplingEngine.attach_ms(200, onSamplingTick);

    Serial.println("MineGuard ready. loop() is empty — everything is event/timer driven.");
}

void loop() {
    // Intentionally empty: collision/SOS are interrupt-driven, and sensor
    // sampling runs on the Ticker. Nothing to poll here.
}
