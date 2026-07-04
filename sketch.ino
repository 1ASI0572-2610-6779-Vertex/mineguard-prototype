/**
 * @file sketch.ino
 * @brief MineGuard Wokwi prototype using Modest IoT Nano Framework.
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include <Wire.h>
#include "MineGuardDevice.h"

MineGuardDevice* device = nullptr;

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22); // I2C: SDA=21, SCL=22

    Serial.println("MineGuard prototype starting...");
    device = new MineGuardDevice();
    device->initialize();
}

void loop() {
    // Runtime work is handled by the Modest IoT framework tasks and scheduler.
    vTaskDelay(pdMS_TO_TICKS(60000));
}
