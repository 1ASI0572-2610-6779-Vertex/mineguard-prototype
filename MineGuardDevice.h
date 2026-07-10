#ifndef MINEGUARD_DEVICE_H
#define MINEGUARD_DEVICE_H

/**
 * @file MineGuardDevice.h
 * @brief MineGuard embedded device built on the Modest IoT Nano Framework.
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include "GPSSensor.h"
#include "HeartRateSensor.h"
#include "MineGuardBuzzer.h"
#include "SafetyPolicy.h"

class MineGuardDevice : public Device {
private:
    Led dangerLed;
    Led safeLed;
    MineGuardBuzzer buzzer;
    CharacterLcdDisplay display;

    UltrasonicSensor frontDistanceSensor;
    UltrasonicSensor backDistanceSensor;
    UltrasonicSensor leftDistanceSensor;
    UltrasonicSensor rightDistanceSensor;
    HeartRateSensor pulseSensor;
    Button collisionButton;
    Button sosButton;
    GPSSensor gpsSensor;

    SafetyPolicy safetyPolicy;

    unsigned long collisionHoldUntil;
    unsigned long sosHoldUntil;

    String lastDisplaySignature;
    bool lastDangerState;
    bool indicatorsInitialized;

    void refreshIndicators(bool danger);
    float getNearestDistance(const char*& nearestDirection);
    float normalizeDistance(float distanceCm) const;
    void refreshDisplay(const MineGuardStatus& status, int bpm, float distanceCm, const char* nearestDirection);
    MineGuardStatus evaluateCurrentStatus();
    String buildHeader(const MineGuardStatus& status, const char* nearestDirection) const;
    String buildDataLine(const MineGuardStatus& status, int bpm, float distanceCm) const;

public:
    static const int RED_LED_PIN = 15;
    static const int GREEN_LED_PIN = 2;
    static const int BUZZER_PIN = 23;
    static const uint8_t LCD_ADDRESS = 0x27;

    static const int FRONT_DISTANCE_TRIG_PIN = 5;
    static const int FRONT_DISTANCE_ECHO_PIN = 18;
    static const int BACK_DISTANCE_TRIG_PIN = 19;
    static const int BACK_DISTANCE_ECHO_PIN = 34;
    static const int LEFT_DISTANCE_TRIG_PIN = 25;
    static const int LEFT_DISTANCE_ECHO_PIN = 35;
    static const int RIGHT_DISTANCE_TRIG_PIN = 26;
    static const int RIGHT_DISTANCE_ECHO_PIN = 27;

    static const int PULSE_PIN = 32;
    static const int COLLISION_PIN = 4;
    static const int SOS_PIN = 13;
    static const int GPS_SERIAL = 2;
    static const int GPS_RX_PIN = 16;
    static const int GPS_TX_PIN = 17;

    static const unsigned long COLLISION_HOLD_MS = 3000;
    static const unsigned long SOS_HOLD_MS = 5000;
    MineGuardDevice();

    bool initialize();
    void on(Event event) override;
};

#endif // MINEGUARD_DEVICE_H
