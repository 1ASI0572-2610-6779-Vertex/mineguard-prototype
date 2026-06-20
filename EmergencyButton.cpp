#include "EmergencyButton.h"
#include <Arduino.h>

const Event EmergencyButton::SOS_PRESSED_EVENT = Event(EmergencyButton::SOS_PRESSED_EVENT_ID);

void IRAM_ATTR EmergencyButton::isrTrampoline(void* arg) {
    EmergencyButton* self = static_cast<EmergencyButton*>(arg);
    self->triggeredFlag = true;
}

EmergencyButton::EmergencyButton(int pin, EventHandler* eventHandler)
    : Sensor(pin, eventHandler), triggeredFlag(false), pressed(false) {
    pinMode(pin, INPUT_PULLUP);
    attachInterruptArg(pin, isrTrampoline, this, FALLING);
}

void EmergencyButton::poll() {
    noInterrupts();
    bool captured = triggeredFlag;
    triggeredFlag = false;
    interrupts();

    if (captured) {
        pressed = true;
        on(SOS_PRESSED_EVENT);
    } else {
        pressed = false;
    }
}

bool EmergencyButton::isPressed() const {
    return pressed;
}
