#include "DHTModule.h"
#include "board.h"

DHTModule::DHTModule(int pin, MQTTModule* mqtt) : mqtt(mqtt) {
    dht = new DHT_Unified(pin, DHT22);
}

DHTModule::~DHTModule() {
    delete dht;
}

void DHTModule::begin() {
    dht->begin();
    Serial.println(F("DHT22 Unified Sensor initialized"));

    // Print temperature sensor details.
    sensor_t sensor;
    dht->temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print(F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
    Serial.print(F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
    Serial.print(F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));

    // Print humidity sensor details.
    dht->humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
    Serial.print(F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print(F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print(F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print(F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
    Serial.print(F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
    Serial.print(F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
    Serial.println(F("------------------------------------"));

    // Set delay between sensor readings based on sensor details.
    delayMS = sensor.min_delay / 1000;
}

float DHTModule::readTemperature() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    if (isnan(event.temperature)) {
        Serial.println(F("Error reading temperature!"));
        return NAN;
    }
    return event.temperature;
}

float DHTModule::readHumidity() {
    sensors_event_t event;
    dht->humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
        Serial.println(F("Error reading humidity!"));
        return NAN;
    }
    return event.relative_humidity;
}

void DHTModule::publishTemperatureHumidity() {
    float temperature = readTemperature();
    float humidity = readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
        String message = "{\"temperature\":" + String(temperature, 2) +
                         ",\"humidity\":" + String(humidity, 2) +
                         ",\"timestamp\":" + String(millis()) + "}";

        if (mqtt->publishSensor(message)) {
            Serial.print(F("DHT22 published: Temperature: "));
            Serial.print(temperature);
            Serial.print(F("°C, Humidity: "));
            Serial.print(humidity);
            Serial.println(F("%"));
        } else {
            Serial.println(F("Failed to publish DHT22 data"));
        }
    }
}