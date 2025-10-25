#include <Arduino.h>
#include "board.h"
#include "NetworkController.h"
#include "MQTTModule.h"
#include "ButtonModule.h"
#include "SoilMoistureModule.h"
#include "DHTModule.h"
#include "ConfigLoader.h"

NetworkController* netManager;
MQTTModule* mqtt;
ButtonModule* button;
SoilMoistureModule* soilSensor;
DHTModule* dhtSensor;

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

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Load configuration from LittleFS
    if (!ConfigLoader::loadConfig()) {
        Serial.println("Failed to load config, using defaults");
    }

    netManager = new NetworkController();
    mqtt = new MQTTModule(netManager);
    mqtt->setMQTT5(true);
    button = new ButtonModule(BUTTON_PIN, mqtt);
    soilSensor = new SoilMoistureModule(SOIL_MOISTURE_POWER_PIN, SOIL_MOISTURE_ADC_PIN, mqtt);
    dhtSensor = new DHTModule(DHT_PIN, mqtt);

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
    // Ethernet not available on this board, skip setup
    // byte mac[6];
    // ConfigLoader::getEthernetMAC(mac);
    // netManager->setEthernetConfig(mac, ConfigLoader::getEthernetIP(), ConfigLoader::getEthernetGateway(), ConfigLoader::getEthernetSubnet());
    // LTE not available on this board, skip setup
    // netManager->setLTEAPN(ConfigLoader::getLTEAPN(), ConfigLoader::getLTEUser(), ConfigLoader::getLTEPass());

    netManager->setOnConnectedCallback(onConnected);
    netManager->setOnDisconnectedCallback(onDisconnected);

    netManager->begin();

    // Load certificates after network initialization
    delay(100);  // Small delay to ensure network is ready
    mqtt->loadCertsFromSPIFFS();

    // Initialize sensor modules
    button->begin();
    soilSensor->begin();
    dhtSensor->begin();

    // Note: Subscription to command topic happens automatically when MQTT connects
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    static unsigned long lastSensorReading = 0;
    static unsigned long lastStatusUpdate = 0;
    static bool lastMQTTConnected = false;
    static uint32_t sessionCount = 0;

    netManager->update();
    mqtt->update();
    button->update();

    bool mqttConnected = mqtt->isConnected();
    if (mqttConnected && !lastMQTTConnected) {
        sessionCount++;
        unsigned long uptimeSeconds = millis() / 1000;
        String retainedStatus = "{\"session\":" + String(sessionCount) +
                                ",\"uptime_seconds\":" + String(uptimeSeconds) +
                                ",\"timestamp_ms\":" + String(millis()) +
                                "}";
        if (mqtt->publishStatus(retainedStatus, true)) {
            Serial.println("Retained status published");
        } else {
            Serial.println("Failed to publish retained status");
        }
    }
    lastMQTTConnected = mqttConnected;

    // Send heartbeat every 30 seconds
    if (millis() - lastHeartbeat > 30000) {
        if (mqtt->publishHeartbeat()) {
            Serial.println("Heartbeat sent");
        }
        lastHeartbeat = millis();
    }

    // Send sensor data every 10 seconds
    if (millis() - lastSensorReading > 10000) {
        soilSensor->publishMoisture();
        dhtSensor->publishTemperatureHumidity();
        lastSensorReading = millis();
    }

    // Send status update every 60 seconds
    if (millis() - lastStatusUpdate > 60000) {
        String statusMsg = "{\"uptime\":" + String(millis()/1000) +
                          ",\"network\":\"" + (netManager->getState() == CONNECTED ? "connected" : "disconnected") + "\"" +
                          ",\"mqtt\":\"" + (mqtt->isConnected() ? "connected" : "disconnected") + "\"}";
        if (mqtt->publishStatus(statusMsg)) {
            Serial.println("Status update sent");
        }
        lastStatusUpdate = millis();
    }
}