#ifndef COLLISION_SENSOR_H
#define COLLISION_SENSOR_H

/**
 * @file CollisionSensor.h
 * @brief KY-031 shock/collision sensor — interrupt-driven (event-based).
 *
 * Instead of polling, the sensor attaches a hardware interrupt on the KY-031
 * digital output (active LOW). Each impact sets a flag inside the ISR; the
 * captured event is later propagated as COLLISION_DETECTED_EVENT when poll()
 * runs in task context (the ISR itself must stay tiny).
 */

#include "Sensor.h"
#include <Arduino.h>

class CollisionSensor : public Sensor {
private:
    volatile bool triggeredFlag;     ///< Set by the ISR on each impact edge.
    bool collisionDetected;          ///< Last consumed state.

    static void IRAM_ATTR isrTrampoline(void* arg);

public:
    static const int COLLISION_DETECTED_EVENT_ID = 13;
    static const Event COLLISION_DETECTED_EVENT;

    /**
     * @brief Constructs the collision sensor and attaches its interrupt.
     * @param pin Digital input GPIO connected to the KY-031 DO output.
     * @param eventHandler Optional upstream handler (default: nullptr).
     */
    CollisionSensor(int pin, EventHandler* eventHandler = nullptr);

    /** @brief Consumes any interrupt-captured impact and emits the event. */
    void poll();

    /** @brief Returns whether an impact was captured on the last poll. */
    bool getCollisionStatus() const;
};

#endif // COLLISION_SENSOR_H
