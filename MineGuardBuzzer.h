#ifndef MINEGUARD_BUZZER_H
#define MINEGUARD_BUZZER_H

/**
 * @file MineGuardBuzzer.h
 * @brief Passive buzzer actuator for the MineGuard prototype.
 *
 * Drives a restrained alarm tone so proximity, collision, and SOS alerts are
 * noticeable without using the default high-pitch buzzer sound.
 */

#include <ModestIoT.h>

class MineGuardBuzzer : public Actuator {
private:
    bool active;            ///< Current buzzer state.
    unsigned int frequency; ///< Alarm tone frequency in Hz.

protected:
    void executeCommand(Command command) override;

public:
    static const int TURN_ON_COMMAND_IDENTIFIER = 300;
    static const int TURN_OFF_COMMAND_IDENTIFIER = 301;

    static const Command TURN_ON_COMMAND;
    static const Command TURN_OFF_COMMAND;

    /**
     * @brief Constructs the prototype buzzer actuator.
     * @param pin Output GPIO pin connected to the buzzer.
     * @param frequency Alarm tone frequency in Hz.
     * @param parentHandler Optional parent command handler.
     */
    MineGuardBuzzer(int pin, unsigned int frequency, CommandHandler* parentHandler = nullptr);

    /** @brief Returns whether the buzzer is currently sounding. */
    bool isActive() const;
};

#endif // MINEGUARD_BUZZER_H
