#ifndef BUTTON_MODULE_H
#define BUTTON_MODULE_H

#include <Arduino.h>
#include "MQTTModule.h"

class ButtonModule {
private:
    int pin;
    MQTTModule* mqtt;
    bool lastStableState;
    bool lastReading;
    unsigned long lastDebounceTime;
    const unsigned long debounceDelay = 50; // 50ms debounce
    bool isPressed;

    void publishEvent(const String& event);

public:
    ButtonModule(int pin, MQTTModule* mqtt);
    void begin();
    void update();
};

#endif // BUTTON_MODULE_H