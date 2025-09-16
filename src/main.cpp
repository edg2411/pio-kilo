#include <Arduino.h>
#include <WiFi.h>
#include "NetworkController.h"
#include "ConfigLoader.h"
#include "board.h"

NetworkController* netManager;
WiFiServer server(80);
bool relayState = false;
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

    // Start HTTP server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    netManager->update();

    // Handle HTTP clients
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected");
        String request = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                request += c;
                if (c == '\n') break; // End of line
            }
        }
        // Parse request
        if (request.indexOf("GET /open") >= 0) {
            relayState = true;
            digitalWrite(RELAY_PIN, HIGH);
            digitalWrite(LED_PIN, HIGH);
            Serial.println("Relay opened");
        } else if (request.indexOf("GET /close") >= 0) {
            relayState = false;
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
            Serial.println("Relay closed");
        }
        // Send response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        client.println("<head><title>Relay Control</title></head>");
        client.println("<body>");
        client.println("<h1>Relay State: " + String(relayState ? "OPEN" : "CLOSE") + "</h1>");
        client.println("<a href=\"/open\"><button>Open Relay</button></a>");
        client.println("<a href=\"/close\"><button>Close Relay</button></a>");
        client.println("</body>");
        client.println("</html>");
        client.stop();
        Serial.println("Client disconnected");
    }

    // Handle button press
    if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonPress > debounceDelay) {
        relayState = !relayState;
        digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
        digitalWrite(LED_PIN, relayState ? HIGH : LOW);
        Serial.println("Button pressed, relay toggled to " + String(relayState ? "OPEN" : "CLOSE"));
        lastButtonPress = millis();
    }

    delay(100);
}