#include "HeartRateSensor.h"

HeartRateSensor::HeartRateSensor(int signalPin, EventHandler* parentHandler)
    : Sensor(signalPin, parentHandler),
      samplingTaskHandle(nullptr),
      currentBpm(0),
      lastBeatMillis(0),
      signalLost(true),
      lastReportedBpm(0) {
    pinMode(pin, INPUT);

    xTaskCreatePinnedToCore(
        samplingTaskEntry,
        "HeartRateSampler",
        2048,
        this,
        1,
        &samplingTaskHandle,
        1);

    Serial.println("HeartRateSensor: sampling task started");
}

void HeartRateSensor::samplingTaskEntry(void* param) {
    static_cast<HeartRateSensor*>(param)->samplingLoop();
}

void HeartRateSensor::samplingLoop() {
    bool aboveThreshold = false;
    unsigned long previousBeatMillis = 0;

    for (;;) {
        int raw = analogRead(pin);
        unsigned long now = millis();

        if (!aboveThreshold && raw >= RISING_EDGE_THRESHOLD) {
            aboveThreshold = true;

            if (now - lastBeatMillis >= REFRACTORY_MS) {
                if (previousBeatMillis != 0) {
                    unsigned long ibi = now - previousBeatMillis;
                    if (ibi > 0) {
                        int computedBpm = static_cast<int>(60000UL / ibi);
                        if (computedBpm >= MIN_VALID_BPM && computedBpm <= MAX_VALID_BPM) {
                            currentBpm = computedBpm;
                        }
                    }
                }
                previousBeatMillis = now;
                lastBeatMillis = now;
                signalLost = false;
            }
        } else if (aboveThreshold && raw < RISING_EDGE_THRESHOLD) {
            aboveThreshold = false;
        }

        if (now - lastBeatMillis > SIGNAL_TIMEOUT_MS) {
            currentBpm = 0;
            signalLost = true;
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

void HeartRateSensor::processEvent(Event& event) {
    if (event.identifier != MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER) {
        event.identifier = -1;
        return;
    }

    int snapshotBpm = currentBpm;

    if (snapshotBpm != lastReportedBpm) {
        lastReportedBpm = snapshotBpm;
        event = Event(DATA_READ_EVENT_IDENTIFIER, pin);
        return;
    }

    event.identifier = -1;
}

int HeartRateSensor::getBeatsPerMinute() const {
    return currentBpm;
}

bool HeartRateSensor::hasSignal() const {
    return !signalLost;
}
