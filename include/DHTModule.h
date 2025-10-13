#ifndef DHT_MODULE_H
#define DHT_MODULE_H

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "MQTTModule.h"

class DHTModule {
private:
    DHT_Unified* dht;
    MQTTModule* mqtt;
    uint32_t delayMS;

    float readTemperature();
    float readHumidity();

public:
    DHTModule(int pin, MQTTModule* mqtt);
    ~DHTModule();
    void begin();
    void publishTemperatureHumidity();
};

#endif // DHT_MODULE_H