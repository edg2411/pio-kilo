#include <Arduino.h>
#include <WiFi.h>
#include "NetworkController.h"
#include "ConfigLoader.h"
#include "board.h"
#include "WebServerModule.h"

NetworkController* netManager;
WebServerModule* webServer;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

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

    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
    digitalWrite(LED_PIN, LOW);    // Initial LED off
    digitalWrite(RELAY_PIN, LOW);  // Initial relay off

    // Load configuration from LittleFS
    if (!ConfigLoader::loadConfig()) {
        Serial.println("Failed to load config, using defaults");
    }

    netManager = new NetworkController();

    // Configure Ethernet static IP if enabled
    if (ConfigLoader::getEthernetStaticIPEnabled()) {
        Serial.println("Ethernet static IP enabled in config");
        netManager->setEthernetStaticIP(
            ConfigLoader::getEthernetStaticIP(),
            ConfigLoader::getEthernetStaticGateway(),
            ConfigLoader::getEthernetStaticSubnet(),
            ConfigLoader::getEthernetStaticDNS1(),
            ConfigLoader::getEthernetStaticDNS2()
        );
    } else {
        // Use DHCP for Ethernet
        Serial.println("Ethernet using DHCP");
        byte mac[6];
        ConfigLoader::getEthernetMAC(mac);
        netManager->setEthernetConfig(mac, ConfigLoader::getEthernetIP(), ConfigLoader::getEthernetGateway(), ConfigLoader::getEthernetSubnet());
    }

    netManager->setOnConnectedCallback(onConnected);
    netManager->setOnDisconnectedCallback(onDisconnected);

    netManager->begin();

    // Start web server
    webServer = new WebServerModule(80, RELAY_PIN, LED_PIN);
    webServer->begin();
}

void loop() {
    netManager->update();

    // Handle web server clients
    webServer->handleClient();

    // Handle button press
    if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        bool newState = !webServer->getRelayState();
        webServer->setRelayState(newState);
        digitalWrite(RELAY_PIN, newState ? HIGH : LOW);
        digitalWrite(LED_PIN, newState ? HIGH : LOW);
        Serial.println("Button pressed, door toggled to " + String(newState ? "OPEN" : "CLOSED"));
        lastButtonPress = millis();
    }

    delay(10);
}