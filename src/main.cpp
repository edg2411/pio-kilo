#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "NetworkController.h"
#include "MQTTModule.h"
#include "ConfigLoader.h"

#define DHTPIN  26     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22 (AM2302), AM2321

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
sensor_t sensor;

NetworkController* netManager;
MQTTModule* mqtt;

void onConnected(NetInterface interface) {
    Serial.print("Connected via ");
    switch (interface) {
        case ETHERNET: Serial.println("Ethernet"); break;
        case WIFI: Serial.println("WiFi"); break;
        case LTE: Serial.println("LTE"); break;
    }
}

void onDisconnected(NetInterface interface) {
    Serial.print("Disconnected from ");
    switch (interface) {
        case ETHERNET: Serial.println("Ethernet"); break;
        case WIFI: Serial.println("WiFi"); break;
        case LTE: Serial.println("LTE"); break;
    }
}

void showSensorInfo(){
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    dht.begin();
    delay(2000);  // Allow sensor to stabilize after initialization
    showSensorInfo();

    // Load configuration from LittleFS
    if (!ConfigLoader::loadConfig()) {
        Serial.println("Failed to load config, using defaults");
    }

    netManager = new NetworkController();
    mqtt = new MQTTModule(netManager);

    // Set MQTT broker from config
    mqtt->setBroker(ConfigLoader::getMQTTBroker(), ConfigLoader::getMQTTPort());
    mqtt->setCredentials(ConfigLoader::getMQTTClientId(), ConfigLoader::getMQTTUsername(), ConfigLoader::getMQTTPassword());

    // Set MQTT topics from config
    mqtt->setTopics(
        ConfigLoader::getMQTTStatusTopic(),
        ConfigLoader::getMQTTCommandTopic(),
        ConfigLoader::getMQTTSensorTopic(),
        ConfigLoader::getMQTTHeartbeatTopic()
    );

    // Set network credentials from config
    netManager->setWiFiCredentials(ConfigLoader::getWiFiSSID(), ConfigLoader::getWiFiPassword());

    // Configure WiFi static IP if enabled
    if (ConfigLoader::getWiFiStaticIPEnabled()) {
        Serial.println("WiFi static IP enabled in config");
        netManager->setWiFiStaticIP(
            ConfigLoader::getWiFiStaticIP(),
            ConfigLoader::getWiFiStaticGateway(),
            ConfigLoader::getWiFiStaticSubnet(),
            ConfigLoader::getWiFiStaticDNS1(),
            ConfigLoader::getWiFiStaticDNS2()
        );
    }
    // // Configure Ethernet static IP if enabled
    // if (ConfigLoader::getEthernetStaticIPEnabled()) {
    //     Serial.println("Ethernet static IP enabled in config");
    //     netManager->setEthernetStaticIP(
    //         ConfigLoader::getEthernetStaticIP(),
    //         ConfigLoader::getEthernetStaticGateway(),
    //         ConfigLoader::getEthernetStaticSubnet(),
    //         ConfigLoader::getEthernetStaticDNS1(),
    //         ConfigLoader::getEthernetStaticDNS2()
    //     );
    // } else {
    //     // Use DHCP for Ethernet
    //     Serial.println("Ethernet using DHCP");
    //     byte mac[6];
    //     ConfigLoader::getEthernetMAC(mac);
    //     netManager->setEthernetConfig(mac, ConfigLoader::getEthernetIP(), ConfigLoader::getEthernetGateway(), ConfigLoader::getEthernetSubnet());
    // }
    // netManager->setLTEAPN(ConfigLoader::getLTEAPN(), ConfigLoader::getLTEUser(), ConfigLoader::getLTEPass());

    netManager->setOnConnectedCallback(onConnected);
    netManager->setOnDisconnectedCallback(onDisconnected);

    netManager->begin();

    // Load certificates after network initialization
    delay(100);  // Small delay to ensure network is ready
    mqtt->loadCertsFromSPIFFS();

    // Note: Subscription to command topic happens automatically when MQTT connects
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastSensorReading = 0;
  static unsigned long lastStatusUpdate = 0;

  netManager->update();
  mqtt->update();

  if (millis() - lastHeartbeat >= 30000) {
    if (mqtt->publishHeartbeat()) {
      Serial.println("Heartbeat sent");
    }
    lastHeartbeat = millis();
  }

  if (millis() - lastSensorReading >= 10000) {
    sensors_event_t event;
    float temperature = NAN;
    float humidity = NAN;

    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature)) {
      temperature = event.temperature;
      Serial.print(F("Temperature: "));
      Serial.print(temperature);
      Serial.println(F("°C"));
    } else {
      Serial.println(F("Error reading temperature!"));
    }

    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) {
      humidity = event.relative_humidity;
      Serial.print(F("Humidity: "));
      Serial.print(humidity);
      Serial.println(F("%"));
    } else {
      Serial.println(F("Error reading humidity!"));
    }

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      delay(delayMS);
      return;
    }

    String sensorData = "{\"temperature\":" + String(temperature) +
               ",\"humidity\":" + String(humidity) +
               ",\"timestamp\":" + String(millis()) + "}";
    if (mqtt->publishSensor(sensorData)) {
      Serial.println("Sensor data sent");
    }
    lastSensorReading = millis();
  }

  if (millis() - lastStatusUpdate >= 60000) {
    String statusMsg = "{\"uptime\":" + String(millis()/1000) +
              ",\"network\":\"" + (netManager->getState() == CONNECTED ? "connected" : "disconnected") + "\"" +
              ",\"mqtt\":\"" + (mqtt->isConnected() ? "connected" : "disconnected") + "\"}";
    if (mqtt->publishStatus(statusMsg)) {
      Serial.println("Status update sent");
    }
    lastStatusUpdate = millis();
  }

  delay(100);
}