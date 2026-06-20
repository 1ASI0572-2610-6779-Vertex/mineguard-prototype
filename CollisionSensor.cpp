#include "CollisionSensor.h"
#include <Arduino.h>

const Event CollisionSensor::COLLISION_DETECTED_EVENT = Event(CollisionSensor::COLLISION_DETECTED_EVENT_ID);

void IRAM_ATTR CollisionSensor::isrTrampoline(void* arg) {
    // Keep the ISR minimal: just latch the event flag.
    CollisionSensor* self = static_cast<CollisionSensor*>(arg);
    self->triggeredFlag = true;
}

CollisionSensor::CollisionSensor(int pin, EventHandler* eventHandler)
    : Sensor(pin, eventHandler), triggeredFlag(false), collisionDetected(false) {
    pinMode(pin, INPUT_PULLUP); // KY-031 DO is active-low
    attachInterruptArg(pin, isrTrampoline, this, FALLING);
}

void CollisionSensor::poll() {
    // Consume the interrupt-captured flag atomically.
    noInterrupts();
    bool captured = triggeredFlag;
    triggeredFlag = false;
    interrupts();

    if (captured) {
        collisionDetected = true;
        on(COLLISION_DETECTED_EVENT); // propagate to the device
    } else {
        collisionDetected = false;
    }
}

bool CollisionSensor::getCollisionStatus() const {
    return collisionDetected;
}
