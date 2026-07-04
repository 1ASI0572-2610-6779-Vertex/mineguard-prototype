#include "MineGuardDevice.h"

MineGuardDevice::MineGuardDevice()
    : Device(100, 0),
      dangerLed(RED_LED_PIN, false, this),
      safeLed(GREEN_LED_PIN, true, this),
      buzzer(BUZZER_PIN, 450, this),
      display(LCD_ADDRESS, 16, 2, true, this),
      frontDistanceSensor(FRONT_DISTANCE_TRIG_PIN, FRONT_DISTANCE_ECHO_PIN, this),
      backDistanceSensor(BACK_DISTANCE_TRIG_PIN, BACK_DISTANCE_ECHO_PIN, this),
      leftDistanceSensor(LEFT_DISTANCE_TRIG_PIN, LEFT_DISTANCE_ECHO_PIN, this),
      rightDistanceSensor(RIGHT_DISTANCE_TRIG_PIN, RIGHT_DISTANCE_ECHO_PIN, this),
      pulseSensor(PULSE_PIN, this),
      collisionButton(COLLISION_PIN, 80, this),
      sosButton(SOS_PIN, 80, this),
      gpsSensor(GPS_SERIAL, GPS_RX_PIN, GPS_TX_PIN, this),
      collisionHoldUntil(0),
      sosHoldUntil(0),
      lastDisplaySignature(""),
      lastDangerState(false),
      indicatorsInitialized(false) {
}

bool MineGuardDevice::initialize() {
    Serial.println("Initializing MineGuard with Modest IoT Nano Framework...");

    initializeAsynchronousEngine(10);

    appendSensorToScheduler(&frontDistanceSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&backDistanceSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&leftDistanceSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&rightDistanceSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&pulseSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&collisionButton, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&sosButton, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);
    appendSensorToScheduler(&gpsSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER);

    display.setLineBuffer(0, " MineGuard Ready");
    display.setLineBuffer(1, "Framework active");
    display.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);

    refreshIndicators(false);
    Serial.println("MineGuard ready. Framework scheduler is active.");
    return true;
}

void MineGuardDevice::on(Event event) {
    if (event.identifier == Button::BUTTON_PRESSED_EVENT_IDENTIFIER) {
        if (event.sourceId == COLLISION_PIN) {
            Serial.println(">> EVENT: COLLISION DETECTED");
            collisionHoldUntil = millis() + COLLISION_HOLD_MS;
        } else if (event.sourceId == SOS_PIN) {
            Serial.println(">> EVENT: SOS PRESSED");
            sosHoldUntil = millis() + SOS_HOLD_MS;
        }
    }

    MineGuardStatus status = evaluateCurrentStatus();
    int bpm = pulseSensor.getBeatsPerMinute();
    const char* nearestDirection = "";
    float distanceCm = getNearestDistance(nearestDirection);

    if (status.danger != lastDangerState || !indicatorsInitialized) {
        refreshIndicators(status.danger);
        lastDangerState = status.danger;
        indicatorsInitialized = true;
    }

    refreshDisplay(status, bpm, distanceCm, nearestDirection);
}

MineGuardStatus MineGuardDevice::evaluateCurrentStatus() {
    int bpm = pulseSensor.getBeatsPerMinute();
    const char* nearestDirection = "";
    float distanceCm = getNearestDistance(nearestDirection);

    MineGuardSnapshot snapshot = {
        distanceCm,
        bpm,
        bpm > 0,
        millis() < collisionHoldUntil,
        millis() < sosHoldUntil
    };
    return safetyPolicy.evaluate(snapshot);
}

float MineGuardDevice::normalizeDistance(float distanceCm) const {
    if (distanceCm < 0.0f) {
        return 999.0f;
    }
    return distanceCm;
}

float MineGuardDevice::getNearestDistance(const char*& nearestDirection) {
    float frontDistanceCm = normalizeDistance(frontDistanceSensor.getDistanceInCentimeters());
    float backDistanceCm = normalizeDistance(backDistanceSensor.getDistanceInCentimeters());
    float leftDistanceCm = normalizeDistance(leftDistanceSensor.getDistanceInCentimeters());
    float rightDistanceCm = normalizeDistance(rightDistanceSensor.getDistanceInCentimeters());

    float nearestDistanceCm = frontDistanceCm;
    nearestDirection = "FRONT";

    if (backDistanceCm < nearestDistanceCm) {
        nearestDistanceCm = backDistanceCm;
        nearestDirection = "BACK";
    }
    if (leftDistanceCm < nearestDistanceCm) {
        nearestDistanceCm = leftDistanceCm;
        nearestDirection = "LEFT";
    }
    if (rightDistanceCm < nearestDistanceCm) {
        nearestDistanceCm = rightDistanceCm;
        nearestDirection = "RIGHT";
    }

    return nearestDistanceCm;
}

void MineGuardDevice::refreshIndicators(bool danger) {
    if (danger) {
        dangerLed.handle(Led::TURN_ON_COMMAND);
        safeLed.handle(Led::TURN_OFF_COMMAND);
        buzzer.handle(MineGuardBuzzer::TURN_ON_COMMAND);
    } else {
        dangerLed.handle(Led::TURN_OFF_COMMAND);
        safeLed.handle(Led::TURN_ON_COMMAND);
        buzzer.handle(MineGuardBuzzer::TURN_OFF_COMMAND);
    }
}

String MineGuardDevice::buildHeader(const MineGuardStatus& status, const char* nearestDirection) const {
    char header[17];
    if (status.sosAlert) {
        snprintf(header, sizeof(header), "!SOS ACTIVATED! ");
    } else if (status.collisionAlert) {
        snprintf(header, sizeof(header), "!COLLISION ALERT");
    } else if (status.proximityAlert) {
        snprintf(header, sizeof(header), "!NEAR %-5s!  ", nearestDirection);
    } else if (status.highHeartRateAlert) {
        snprintf(header, sizeof(header), "!HIGH HEART RATE");
    } else if (status.fatigueAlert) {
        snprintf(header, sizeof(header), "!FATIGUE ALERT! ");
    } else {
        snprintf(header, sizeof(header), "MineGuard [OK]  ");
    }
    return String(header);
}

String MineGuardDevice::buildDataLine(const MineGuardStatus& status, int bpm, float distanceCm) const {
    char buffer[17];
    if (status.sosAlert) {
        if (gpsSensor.hasFix()) {
            snprintf(buffer, sizeof(buffer), "G:%2.2f,%2.2f", gpsSensor.getLatitude(), gpsSensor.getLongitude());
        } else {
            snprintf(buffer, sizeof(buffer), "GPS: Searching..");
        }
    } else if (bpm > 0) {
        snprintf(buffer, sizeof(buffer), "D:%3dcm P:%3dbpm", (int)distanceCm, bpm);
    } else {
        snprintf(buffer, sizeof(buffer), "D:%3dcm P:--- ", (int)distanceCm);
    }
    return String(buffer);
}

void MineGuardDevice::refreshDisplay(const MineGuardStatus& status, int bpm, float distanceCm, const char* nearestDirection) {
    String header = buildHeader(status, nearestDirection);
    String dataLine = buildDataLine(status, bpm, distanceCm);
    String signature = header + "|" + dataLine + (status.danger ? "|D" : "|S");

    if (signature == lastDisplaySignature) {
        return;
    }

    display.setLineBuffer(0, header.c_str());
    display.setLineBuffer(1, dataLine.c_str());
    display.handle(CharacterLcdDisplay::UPDATE_TEXT_COMMAND);
    lastDisplaySignature = signature;

    Serial.print("[CHANGE] ");
    Serial.print(header);
    Serial.print(" | ");
    Serial.println(dataLine);
}
