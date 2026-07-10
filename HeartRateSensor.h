#ifndef HEART_RATE_SENSOR_H
#define HEART_RATE_SENSOR_H

/**
 * @file HeartRateSensor.h
 * @brief Custom pulse sensor for the MineGuard prototype, independent from
 *        the ModestIoT framework's built-in PulseSensor.
 *
 * Samples the analog SIG pin on its own FreeRTOS task (decoupled from the
 * framework's generic MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER scheduler tick,
 * which is shared with the ultrasonic/button/GPS sensors and is too slow for
 * reliable peak detection). Detects rising-edge threshold crossings to
 * compute the inter-beat interval and derives BPM from it, the same basic
 * technique used by real pulse sensor libraries.
 */

#include <Arduino.h>
#include <ModestIoT.h>

class HeartRateSensor : public Sensor {
private:
    static const uint32_t SAMPLE_INTERVAL_MS = 10;   ///< ADC polling period.
    static const int RISING_EDGE_THRESHOLD = 700;    ///< Raw ADC crossing point (0-4095).
    static const uint32_t REFRACTORY_MS = 250;        ///< Min. time between beats (~240 BPM cap).
    static const uint32_t SIGNAL_TIMEOUT_MS = 2000;   ///< No-beat window before reporting 0 BPM.
    static const int MIN_VALID_BPM = 30;
    static const int MAX_VALID_BPM = 220;

    TaskHandle_t samplingTaskHandle;

    volatile int currentBpm;
    volatile unsigned long lastBeatMillis;
    volatile bool signalLost;

    int lastReportedBpm;

    static void samplingTaskEntry(void* param);
    void samplingLoop();

protected:
    void processEvent(Event& event) override;

public:
    /**
     * @brief Constructs the pulse sensor and starts its background sampling task.
     * @param signalPin Analog-capable GPIO connected to the sensor's SIG pin.
     * @param parentHandler Optional parent event handler (device).
     */
    explicit HeartRateSensor(int signalPin, EventHandler* parentHandler = nullptr);

    /** @brief Latest computed heart rate in beats per minute (0 if no signal). */
    int getBeatsPerMinute() const;

    /** @brief Whether a beat has been seen within the signal timeout window. */
    bool hasSignal() const;
};

#endif // HEART_RATE_SENSOR_H
