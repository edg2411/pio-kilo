#include "SoilMoistureModule.h"
#include "board.h"
#include "Logging.h"

SoilMoistureModule::SoilMoistureModule(int powerPin, int adcPin, MQTTModule* mqtt, int dryValue, int wetValue)
    : powerPin(powerPin), adcPin(adcPin), mqtt(mqtt), dryValue(dryValue), wetValue(wetValue) {}

void SoilMoistureModule::begin() {
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, LOW);  // Start with sensor powered off
    pinMode(adcPin, INPUT);
    LOGI(LogModule::SoilMoistureModule, "Initialized (Power pin: %d, ADC pin: %d)", powerPin, adcPin);
    LOGI(LogModule::SoilMoistureModule, "Calibration Dry=%d Wet=%d", dryValue, wetValue);
}

void SoilMoistureModule::powerOn() {
    digitalWrite(powerPin, HIGH);
    delay(100);  // Allow sensor to stabilize
}

void SoilMoistureModule::powerOff() {
    digitalWrite(powerPin, LOW);
}

int SoilMoistureModule::readADC() {
    powerOn();
    int value = analogRead(adcPin);
    powerOff();
    return value;
}

int SoilMoistureModule::calculateMoisturePercentage(int adcValue) {
    // Map ADC value to percentage (0-100%)
    // Assuming dryValue > wetValue (higher ADC when dry)
    if (adcValue >= dryValue) return 0;
    if (adcValue <= wetValue) return 100;

    return map(adcValue, dryValue, wetValue, 0, 100);
}

int SoilMoistureModule::getMoistureLevel() {
    int adcValue = readADC();
    return calculateMoisturePercentage(adcValue);
}

void SoilMoistureModule::publishMoisture() {
    int moisture = getMoistureLevel();
    int adcValue = readADC();

    String message = "{\"soil_moisture\":" + String(moisture) +
                     ",\"adc_value\":" + String(adcValue) +
                     ",\"timestamp\":" + String(millis()) + "}";

    if (mqtt->publishSensor(message)) {
        LOGI(LogModule::SoilMoistureModule, "Soil moisture published: %d%% (ADC: %d)", moisture, adcValue);
    } else {
        LOGE(LogModule::SoilMoistureModule, "Failed to publish soil moisture");
    }
}