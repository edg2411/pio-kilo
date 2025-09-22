#include "BuzzerModule.h"

BuzzerModule::BuzzerModule() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    isBeeping = false;
    beepCount = 0;
    maxBeeps = 0;
    isLongBeep = false;
    beepPause = BEEP_PAUSE;
}

BuzzerModule::~BuzzerModule() {
    digitalWrite(BUZZER_PIN, LOW);
}

void BuzzerModule::beepSetupComplete() {
    startBeepSequence(3, false, true);  // 3 startup beeps
}

void BuzzerModule::beepDoorOpen() {
    startBeepSequence(1, false, false);  // 1 short beep
}

void BuzzerModule::beepDoorClose() {
    startBeepSequence(1, true, false);   // 1 long beep
}

void BuzzerModule::beepToggle() {
    startBeepSequence(1, false, false);  // 1 short beep
}

void BuzzerModule::update() {
    if (!isBeeping) return;

    unsigned long currentTime = millis();

    if (currentTime - beepStartTime >= beepDuration) {
        // Turn off buzzer
        digitalWrite(BUZZER_PIN, LOW);

        beepCount++;

        if (beepCount >= maxBeeps) {
            // Sequence complete
            stopBeeping();
        } else {
            // Start next beep
            if (isLongBeep && beepCount == maxBeeps - 1) {
                // Last beep is long
                beepDuration = LONG_BEEP_DURATION;
            } else {
                beepDuration = SHORT_BEEP_DURATION;
            }

            beepStartTime = currentTime + beepPause;
            // Wait for pause, then turn on buzzer
            if (currentTime >= beepStartTime) {
                digitalWrite(BUZZER_PIN, HIGH);
                beepStartTime = currentTime;
            }
        }
    }
}

void BuzzerModule::startBeepSequence(int count, bool hasLongBeep, bool isStartup) {
    if (isBeeping) return;  // Already beeping

    maxBeeps = count;
    beepCount = 0;
    isLongBeep = hasLongBeep;
    if (isStartup) {
        beepDuration = STARTUP_BEEP_DURATION;
        beepPause = STARTUP_PAUSE;
    } else {
        beepDuration = hasLongBeep ? LONG_BEEP_DURATION : SHORT_BEEP_DURATION;
        beepPause = BEEP_PAUSE;
    }
    beepStartTime = millis();
    isBeeping = true;

    // Start first beep
    digitalWrite(BUZZER_PIN, HIGH);
}

void BuzzerModule::stopBeeping() {
    digitalWrite(BUZZER_PIN, LOW);
    isBeeping = false;
    beepCount = 0;
    maxBeeps = 0;
    isLongBeep = false;
    beepPause = BEEP_PAUSE;
}