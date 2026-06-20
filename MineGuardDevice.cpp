#include "MineGuardDevice.h"
#include <Arduino.h>

MineGuardDevice::MineGuardDevice()
    : dangerLed(RED_LED_PIN, false, this),
      safeLed(GREEN_LED_PIN, true, this),
      buzzer(BUZZER_PIN, 1000, this),
      display(LCD_ADDRESS, 16, 2, this),
      proximityFront(FRONT_TRIG_PIN, FRONT_ECHO_PIN, this),
      proximityBack(BACK_TRIG_PIN,  BACK_ECHO_PIN,  this),
      proximityLeft(LEFT_TRIG_PIN,  LEFT_ECHO_PIN,  this),
      proximityRight(RIGHT_TRIG_PIN, RIGHT_ECHO_PIN, this),
      pulse(PULSE_PIN, this),
      collision(COLLISION_PIN, this),
      sos(SOS_PIN, this),
      gps(GPS_SERIAL, GPS_RX_PIN, GPS_TX_PIN, this),
      collisionHoldUntil(0),
      sosHoldUntil(0),
      lastMetricSentAt(0),
      metricPending(false),
      lastSignature(""),
      lastDanger(false),
      indicatorsInitialized(false),
      metricCallback(nullptr) {
}

bool MineGuardDevice::initialize() {
    Serial.println("Initializing MineGuard Device (event-driven)...");
    if (!display.initialize()) {
        Serial.println("Failed to initialize LCD display!");
        return false;
    }
    display.setLine(0, " MineGuard V2.3 ");
    display.setLine(1, " Event-driven   ");
    display.handle(LcdDisplay::REFRESH_COMMAND);
    Serial.println("MineGuard Device ready. Buttons use interrupts; sampling via timer.");
    return true;
}

void MineGuardDevice::setMetricCallback(void (*callback)(double, double, float, int, bool, bool, bool)) {
    metricCallback = callback;
}

float MineGuardDevice::minDistanceWithDirection(const char*& directionOut) {
    float distances[4] = {
        proximityFront.getDistance(),
        proximityBack.getDistance(),
        proximityLeft.getDistance(),
        proximityRight.getDistance()
    };
    const char* labels[4] = { "FRONT", "BACK", "LEFT", "RIGHT" };

    int   nearest = 0;
    float minDist = distances[0];
    for (int i = 1; i < 4; ++i) {
        if (distances[i] < minDist) { minDist = distances[i]; nearest = i; }
    }
    directionOut = labels[nearest];
    return minDist;
}

// ── Event reactions: log + latch momentary events + flag telemetry ──
void MineGuardDevice::on(Event event) {
    if (event == UltrasonicSensor::OBJECT_NEARBY_EVENT) {
        metricPending = true;
    } else if (event == PulseSensor::HIGH_HEART_RATE_EVENT) {
        metricPending = true;
    } else if (event == PulseSensor::FATIGUE_EVENT) {
        metricPending = true;
    } else if (event == CollisionSensor::COLLISION_DETECTED_EVENT) {
        Serial.println(">> EVENT: COLLISION DETECTED (interrupt)");
        collisionHoldUntil = millis() + COLLISION_HOLD_MS;
        metricPending = true;
    } else if (event == EmergencyButton::SOS_PRESSED_EVENT) {
        Serial.println(">> EVENT: SOS PRESSED (interrupt)");
        sosHoldUntil = millis() + SOS_HOLD_MS;
        metricPending = true;
    }
}

void MineGuardDevice::handle(Command command) {
    // Optional actuator-state tracing hook.
}

void MineGuardDevice::refreshIndicators(bool danger) {
    if (danger) {
        dangerLed.handle(Led::TURN_ON_COMMAND);
        safeLed.handle(Led::TURN_OFF_COMMAND);
        buzzer.handle(Buzzer::TURN_ON_COMMAND);
    } else {
        dangerLed.handle(Led::TURN_OFF_COMMAND);
        safeLed.handle(Led::TURN_ON_COMMAND);
        buzzer.handle(Buzzer::TURN_OFF_COMMAND);
    }
}

String MineGuardDevice::buildHeader(bool proximityAlert, bool hrAlert, bool collisionActive,
                                    bool sosActive, int bpm, const char* nearestDir) {
    char header[17];
    if (sosActive)            snprintf(header, sizeof(header), "!SOS ACTIVATED! ");
    else if (collisionActive) snprintf(header, sizeof(header), "!COLLISION ALERT");
    else if (hrAlert && bpm < PulseSensor::FATIGUE_BPM_THRESHOLD)
                              snprintf(header, sizeof(header), "!FATIGUE ALERT! ");
    else if (proximityAlert)  snprintf(header, sizeof(header), "!NEAR %-5s OBJ", nearestDir);
    else if (hrAlert)         snprintf(header, sizeof(header), "!HIGH HEART RATE");
    else                      snprintf(header, sizeof(header), "MineGuard [OK]  ");
    return String(header);
}

String MineGuardDevice::buildDataLine(bool sosActive, int bpm, float minDistance) {
    char buffer[17];
    if (sosActive) {
        if (gps.getLatitude() != 0.0 || gps.getLongitude() != 0.0) {
            snprintf(buffer, sizeof(buffer), "G:%2.2f,%2.2f",
                     gps.getLatitude(), gps.getLongitude());
        } else {
            snprintf(buffer, sizeof(buffer), "GPS: Searching..");
        }
    } else {
        snprintf(buffer, sizeof(buffer), "D:%3dcm P:%3dbpm", (int)minDistance, bpm);
    }
    return String(buffer);
}

// ── Sampling engine tick (driven by the Ticker, NOT the Arduino loop) ──
// Ranges the ultrasonic sensors, reads BPM/GPS, consumes interrupt-captured
// button/collision events, and refreshes outputs ONLY when something changed.
void MineGuardDevice::update() {
    // Range the four ultrasonic sensors (each emits an event only on change)
    proximityFront.scanDistance();
    proximityBack.scanDistance();
    proximityLeft.scanDistance();
    proximityRight.scanDistance();

    // Sample analog heart rate; drain GPS stream
    pulse.scanPulse();
    gps.scanLocation();

    // Consume interrupt-captured digital events (collision / SOS)
    collision.poll();
    sos.poll();

    // Compute current alert state
    const char* nearestDir = "FRONT";
    float minDistance = minDistanceWithDirection(nearestDir);
    int  bpm = pulse.getBpm();
    bool proximityAlert  = (minDistance <= UltrasonicSensor::PROXIMITY_THRESHOLD_CM);
    bool hrAlert         = (bpm >= PulseSensor::HIGH_BPM_THRESHOLD ||
                            bpm <  PulseSensor::FATIGUE_BPM_THRESHOLD);
    bool collisionActive = (millis() < collisionHoldUntil);
    bool sosActive       = (millis() < sosHoldUntil);
    bool danger = proximityAlert || hrAlert || collisionActive || sosActive;

    // Actuators: only switch on a danger-state TRANSITION (event-like)
    if (danger != lastDanger || !indicatorsInitialized) {
        refreshIndicators(danger);
        lastDanger = danger;
        indicatorsInitialized = true;
    }

    // Build display content and compute a change signature
    String header   = buildHeader(proximityAlert, hrAlert, collisionActive, sosActive, bpm, nearestDir);
    String dataLine = buildDataLine(sosActive, bpm, minDistance);
    String signature = header + "|" + dataLine + (danger ? "|D" : "|S");

    // Render + log ONLY when the visible state changes
    if (signature != lastSignature) {
        display.setLine(0, header);
        display.setLine(1, dataLine);
        display.handle(LcdDisplay::REFRESH_COMMAND);

        Serial.print("[CHANGE] ");
        Serial.print(header); Serial.print(" | "); Serial.print(dataLine);
        Serial.print("  (F:"); Serial.print((int)proximityFront.getDistance());
        Serial.print(" B:");   Serial.print((int)proximityBack.getDistance());
        Serial.print(" L:");   Serial.print((int)proximityLeft.getDistance());
        Serial.print(" R:");   Serial.print((int)proximityRight.getDistance());
        Serial.print("cm  GPS:"); Serial.print(gps.getLatitude(), 5);
        Serial.print(","); Serial.print(gps.getLongitude(), 5);
        Serial.println(")");

        lastSignature = signature;
    }

    // Throttled telemetry (only if a callback is set)
    if (metricPending && metricCallback != nullptr &&
        (millis() - lastMetricSentAt >= METRIC_MIN_INTERVAL_MS)) {
        metricCallback(gps.getLatitude(), gps.getLongitude(), minDistance, bpm,
                       proximityAlert, collisionActive, sosActive);
        lastMetricSentAt = millis();
        metricPending = false;
    }
}

// ── Accessors ──
Led& MineGuardDevice::getDangerLed() { return dangerLed; }
Led& MineGuardDevice::getSafeLed() { return safeLed; }
Buzzer& MineGuardDevice::getBuzzer() { return buzzer; }
LcdDisplay& MineGuardDevice::getDisplay() { return display; }
UltrasonicSensor& MineGuardDevice::getFrontSensor() { return proximityFront; }
UltrasonicSensor& MineGuardDevice::getBackSensor()  { return proximityBack; }
UltrasonicSensor& MineGuardDevice::getLeftSensor()  { return proximityLeft; }
UltrasonicSensor& MineGuardDevice::getRightSensor() { return proximityRight; }
PulseSensor& MineGuardDevice::getPulseSensor() { return pulse; }
CollisionSensor& MineGuardDevice::getCollisionSensor() { return collision; }
EmergencyButton& MineGuardDevice::getEmergencyButton() { return sos; }
GPSSensor& MineGuardDevice::getGPSSensor() { return gps; }
