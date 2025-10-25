#ifndef SOIL_MOISTURE_MODULE_H
#define SOIL_MOISTURE_MODULE_H

#include <Arduino.h>
#include "MQTTModule.h"

class SoilMoistureModule {
private:
    int powerPin;
    int adcPin;
    MQTTModule* mqtt;
    int dryValue;   // ADC value when dry (calibration)
    int wetValue;   // ADC value when wet (calibration)

    void powerOn();
    void powerOff();
    int readADC();
    int calculateMoisturePercentage(int adcValue);

public:
    SoilMoistureModule(int powerPin, int adcPin, MQTTModule* mqtt, int dryValue = 4095, int wetValue = 1800);
    void begin();
    int getMoistureLevel();
    void publishMoisture();
};

#endif // SOIL_MOISTURE_MODULE_H