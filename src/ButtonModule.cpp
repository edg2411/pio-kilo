#include "ButtonModule.h"
#include "board.h"
#include "Logging.h"

ButtonModule::ButtonModule(int pin, MQTTModule* mqtt) : pin(pin), mqtt(mqtt), lastStableState(HIGH), lastReading(HIGH), lastDebounceTime(0), isPressed(false) {}

void ButtonModule::begin() {
    pinMode(pin, INPUT_PULLUP);
    lastStableState = digitalRead(pin);
    lastReading = lastStableState;
}

void ButtonModule::update() {
    bool reading = digitalRead(pin);
    unsigned long currentTime = millis();

    if (reading != lastReading) {
        lastDebounceTime = currentTime;
        lastReading = reading;
    }

    if ((currentTime - lastDebounceTime) > debounceDelay) {
        if (reading != lastStableState) {
            lastStableState = reading;

            if (reading == LOW && !isPressed) { // Button pressed
                publishEvent("pressed");
                isPressed = true;
            } else if (reading == HIGH && isPressed) { // Button released
                isPressed = false;
                // No publish on release
            }
        }
    }
}

void ButtonModule::publishEvent(const String& event) {
    String message = "{\"button\":\"" + event + "\",\"timestamp\":" + String(millis()) + "}";
    if (mqtt->publishSensor(message)) {
        LOGI(LogModule::ButtonModule, "Button event published: %s", event.c_str());
    } else {
        LOGE(LogModule::ButtonModule, "Failed to publish button event");
    }
}