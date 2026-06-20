#ifndef EMERGENCY_BUTTON_H
#define EMERGENCY_BUTTON_H

/**
 * @file EmergencyButton.h
 * @brief Manual SOS push button — interrupt-driven (event-based).
 *
 * Attaches a hardware interrupt on the button pin (active LOW, internal pull-up).
 * A press latches a flag inside the ISR; poll() (run in task context) propagates
 * SOS_PRESSED_EVENT. No polling of the pin in a busy loop.
 */

#include "Sensor.h"
#include <Arduino.h>

class EmergencyButton : public Sensor {
private:
    volatile bool triggeredFlag;
    bool pressed;

    static void IRAM_ATTR isrTrampoline(void* arg);

public:
    static const int SOS_PRESSED_EVENT_ID = 14;
    static const Event SOS_PRESSED_EVENT;

    /**
     * @brief Constructs the SOS button and attaches its interrupt.
     * @param pin Digital input GPIO connected to the button.
     * @param eventHandler Optional upstream handler (default: nullptr).
     */
    EmergencyButton(int pin, EventHandler* eventHandler = nullptr);

    /** @brief Consumes any interrupt-captured press and emits the event. */
    void poll();

    /** @brief Returns whether the button was pressed on the last poll. */
    bool isPressed() const;
};

#endif // EMERGENCY_BUTTON_H
