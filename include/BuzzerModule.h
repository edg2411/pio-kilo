#ifndef BUZZER_MODULE_H
#define BUZZER_MODULE_H

#include <Arduino.h>
#include "board.h"

class BuzzerModule {
public:
    BuzzerModule();
    ~BuzzerModule();

    // Beep patterns
    void beepSetupComplete();  // 3 short fast beeps
    void beepDoorOpen();       // 2 short fast beeps
    void beepDoorClose();      // 2 short beeps + 1 longer beep

    // Update function to be called in loop for non-blocking operation
    void update();

private:
    // Timing constants (in milliseconds)
    static const unsigned long SHORT_BEEP_DURATION = 150;
    static const unsigned long LONG_BEEP_DURATION = 500;
    static const unsigned long STARTUP_BEEP_DURATION = 100;
    static const unsigned long BEEP_PAUSE = 200;
    static const unsigned long STARTUP_PAUSE = 100;

    // State variables for non-blocking beeps
    bool isBeeping;
    unsigned long beepStartTime;
    unsigned long beepDuration;
    unsigned long beepPause;
    int beepCount;
    int maxBeeps;
    bool isLongBeep;

    // Private methods
    void startBeepSequence(int count, bool hasLongBeep = false, bool isStartup = false);
    void stopBeeping();
};

#endif // BUZZER_MODULE_H