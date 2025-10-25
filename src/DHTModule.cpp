#include "DHTModule.h"
#include "board.h"
#include "Logging.h"

DHTModule::DHTModule(int pin, MQTTModule* mqtt) : mqtt(mqtt) {
    dht = new DHT_Unified(pin, DHT22);
}

DHTModule::~DHTModule() {
    delete dht;
}

void DHTModule::begin() {
    dht->begin();
    LOGI(LogModule::DHTModule, "DHT22 Unified Sensor initialized");

    // Print temperature sensor details.
    sensor_t sensor;
    dht->temperature().getSensor(&sensor);
    LOGI(LogModule::DHTModule, "------------------------------------");
    LOGI(LogModule::DHTModule, "Temperature Sensor");
    LOGI(LogModule::DHTModule, "Sensor Type: %s", sensor.name);
    LOGI(LogModule::DHTModule, "Driver Ver: %d", sensor.version);
    LOGI(LogModule::DHTModule, "Unique ID: %d", sensor.sensor_id);
    LOGI(LogModule::DHTModule, "Max Value: %.2f°C", sensor.max_value);
    LOGI(LogModule::DHTModule, "Min Value: %.2f°C", sensor.min_value);
    LOGI(LogModule::DHTModule, "Resolution: %.2f°C", sensor.resolution);
    LOGI(LogModule::DHTModule, "------------------------------------");

    // Print humidity sensor details.
    dht->humidity().getSensor(&sensor);
    LOGI(LogModule::DHTModule, "Humidity Sensor");
    LOGI(LogModule::DHTModule, "Sensor Type: %s", sensor.name);
    LOGI(LogModule::DHTModule, "Driver Ver: %d", sensor.version);
    LOGI(LogModule::DHTModule, "Unique ID: %d", sensor.sensor_id);
    LOGI(LogModule::DHTModule, "Max Value: %.2f%%", sensor.max_value);
    LOGI(LogModule::DHTModule, "Min Value: %.2f%%", sensor.min_value);
    LOGI(LogModule::DHTModule, "Resolution: %.2f%%", sensor.resolution);
    LOGI(LogModule::DHTModule, "------------------------------------");

    // Set delay between sensor readings based on sensor details.
    delayMS = sensor.min_delay / 1000;
}

float DHTModule::readTemperature() {
    sensors_event_t event;
    dht->temperature().getEvent(&event);
    if (isnan(event.temperature)) {
    LOGE(LogModule::DHTModule, "Error reading temperature");
        return NAN;
    }
    return event.temperature;
}

float DHTModule::readHumidity() {
    sensors_event_t event;
    dht->humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
    LOGE(LogModule::DHTModule, "Error reading humidity");
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
            LOGI(LogModule::DHTModule, "Published: Temperature %.2f°C, Humidity %.2f%%", temperature, humidity);
        } else {
            LOGE(LogModule::DHTModule, "Failed to publish DHT22 data");
        }
    }
}