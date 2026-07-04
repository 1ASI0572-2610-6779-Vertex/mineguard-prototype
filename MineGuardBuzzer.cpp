#include "MineGuardBuzzer.h"
#include <Arduino.h>

const Command MineGuardBuzzer::TURN_ON_COMMAND = Command(MineGuardBuzzer::TURN_ON_COMMAND_IDENTIFIER);
const Command MineGuardBuzzer::TURN_OFF_COMMAND = Command(MineGuardBuzzer::TURN_OFF_COMMAND_IDENTIFIER);

MineGuardBuzzer::MineGuardBuzzer(int pin, unsigned int frequency, CommandHandler* parentHandler)
    : Actuator(pin, parentHandler), active(false), frequency(frequency) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void MineGuardBuzzer::executeCommand(Command command) {
    if (command == TURN_ON_COMMAND) {
        tone(pin, frequency);
        active = true;
    } else if (command == TURN_OFF_COMMAND) {
        noTone(pin);
        active = false;
    }
}

bool MineGuardBuzzer::isActive() const {
    return active;
}
