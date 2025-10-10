#ifndef SOIL_MOISTURE_MODULE_H
#define SOIL_MOISTURE_MODULE_H

#include <Arduino.h>
#include "MQTTModule.h"

class SoilMoistureModule {
private:
    int pin;
    MQTTModule* mqtt;
    int dryValue;   // ADC value when dry (calibration)
    int wetValue;   // ADC value when wet (calibration)

    int readADC();
    int calculateMoisturePercentage(int adcValue);

public:
    SoilMoistureModule(int pin, MQTTModule* mqtt, int dryValue = 4095, int wetValue = 1800);
    void begin();
    int getMoistureLevel();
    void publishMoisture();
};

#endif // SOIL_MOISTURE_MODULE_H