#ifndef MINEGUARD_DEVICE_H
#define MINEGUARD_DEVICE_H

/**
 * @file MineGuardDevice.h
 * @brief Application mediator for the MineGuard mining-safety IoT node.
 *
 * Mirrors the Modest-IoT "Device" application pattern: it owns every sensor and
 * actuator, reacts to their events via on(), drives local alert indicators, and
 * (optionally) pushes telemetry to an Edge Service through a metric callback.
 *
 * Proximity is now covered in 4 directions (front / back / left / right) using
 * four HC-SR04 ultrasonic sensors, giving the operator 360-degree obstacle awareness.
 */

#include "Device.h"
#include "UltrasonicSensor.h"
#include "PulseSensor.h"
#include "CollisionSensor.h"
#include "EmergencyButton.h"
#include "GPSSensor.h"
#include "Led.h"
#include "Buzzer.h"
#include "LcdDisplay.h"

class MineGuardDevice : public Device {
private:
    // -- Actuators --
    Led         dangerLed;
    Led         safeLed;
    Buzzer      buzzer;
    LcdDisplay  display;

    // -- Sensors --
    UltrasonicSensor proximityFront;
    UltrasonicSensor proximityBack;
    UltrasonicSensor proximityLeft;
    UltrasonicSensor proximityRight;
    PulseSensor      pulse;
    CollisionSensor  collision;
    EmergencyButton  sos;
    GPSSensor        gps;

    // -- Latched alert windows (momentary events held for visibility) --
    unsigned long collisionHoldUntil;
    unsigned long sosHoldUntil;

    // -- Telemetry throttling --
    unsigned long lastMetricSentAt;
    bool          metricPending;

    // -- Reactive change-tracking (render/actuate only on change) --
    String        lastSignature;
    bool          lastDanger;
    bool          indicatorsInitialized;

    // -- Edge Service callback: (lat, lng, minDistanceCm, bpm, proximity, collision, sos) --
    void (*metricCallback)(double, double, float, int, bool, bool, bool);

    float minDistanceWithDirection(const char*& directionOut);
    void refreshIndicators(bool danger);
    String buildHeader(bool proximityAlert, bool hrAlert, bool collisionActive,
                       bool sosActive, int bpm, const char* nearestDir);
    String buildDataLine(bool sosActive, int bpm, float minDistance);

public:
    // -- Actuator / I2C / serial pins --
    static const int RED_LED_PIN    = 2;
    static const int GREEN_LED_PIN  = 15;
    static const int BUZZER_PIN     = 23;
    static const uint8_t LCD_ADDRESS = 0x27;

    // -- Four ultrasonic sensors (TRIG / ECHO) --
    static const int FRONT_TRIG_PIN = 5;   static const int FRONT_ECHO_PIN = 18;
    static const int BACK_TRIG_PIN  = 19;  static const int BACK_ECHO_PIN  = 34;  // 34 = input-only (echo)
    static const int LEFT_TRIG_PIN  = 25;  static const int LEFT_ECHO_PIN  = 35;  // 35 = input-only (echo)
    static const int RIGHT_TRIG_PIN = 26;  static const int RIGHT_ECHO_PIN = 27;

    // -- Other sensors --
    static const int PULSE_PIN      = 32;
    static const int COLLISION_PIN  = 4;
    static const int SOS_PIN        = 13;
    static const int GPS_SERIAL     = 2;  // UART2
    static const int GPS_RX_PIN     = 16;
    static const int GPS_TX_PIN     = 17;

    // -- Behaviour timings --
    static const unsigned long COLLISION_HOLD_MS = 3000;
    static const unsigned long SOS_HOLD_MS       = 5000;
    static const unsigned long METRIC_MIN_INTERVAL_MS = 5000;

    MineGuardDevice();

    // -- Framework contract --
    void on(Event event) override;
    void handle(Command command) override;

    // -- Application lifecycle --
    bool initialize();
    void update();
    void setMetricCallback(void (*callback)(double, double, float, int, bool, bool, bool));

    // -- Accessors --
    Led& getDangerLed();
    Led& getSafeLed();
    Buzzer& getBuzzer();
    LcdDisplay& getDisplay();
    UltrasonicSensor& getFrontSensor();
    UltrasonicSensor& getBackSensor();
    UltrasonicSensor& getLeftSensor();
    UltrasonicSensor& getRightSensor();
    PulseSensor& getPulseSensor();
    CollisionSensor& getCollisionSensor();
    EmergencyButton& getEmergencyButton();
    GPSSensor& getGPSSensor();
};

#endif // MINEGUARD_DEVICE_H
