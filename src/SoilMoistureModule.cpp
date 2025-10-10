#include "SoilMoistureModule.h"
#include "board.h"

SoilMoistureModule::SoilMoistureModule(int pin, MQTTModule* mqtt, int dryValue, int wetValue)
    : pin(pin), mqtt(mqtt), dryValue(dryValue), wetValue(wetValue) {}

void SoilMoistureModule::begin() {
    pinMode(pin, INPUT);
    Serial.println("SoilMoistureModule initialized on pin " + String(pin));
    Serial.println("Calibration: Dry=" + String(dryValue) + ", Wet=" + String(wetValue));
}

int SoilMoistureModule::readADC() {
    return analogRead(pin);
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
        Serial.println("Soil moisture published: " + String(moisture) + "% (ADC: " + String(adcValue) + ")");
    } else {
        Serial.println("Failed to publish soil moisture");
    }
}