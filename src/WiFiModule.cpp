#include "WiFiModule.h"

WiFiModule::WiFiModule() : connected(false), connecting(false), useStaticIP(false) {}

void WiFiModule::setCredentials(const String& ssid, const String& password) {
    this->ssid = ssid;
    this->password = password;
}

void WiFiModule::setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
    this->staticIP = ip;
    this->staticGateway = gateway;
    this->staticSubnet = subnet;
    this->staticDNS1 = dns1;
    this->staticDNS2 = dns2;
}

void WiFiModule::enableStaticIP(bool enable) {
    this->useStaticIP = enable;
}

bool WiFiModule::connect() {
    if (connected) return true;
    if (ssid.isEmpty()) return false;
    if (!connecting) {
        WiFi.begin(ssid.c_str(), password.c_str());
        connecting = true;

        // Configure static IP if enabled
        if (useStaticIP) {
            Serial.println("Configuring WiFi static IP...");
            if (!WiFi.config(staticIP, staticGateway, staticSubnet, staticDNS1, staticDNS2)) {
                Serial.println("Failed to configure static IP");
            }
        }
    }
    return false; // Not yet connected
}

void WiFiModule::disconnect() {
    WiFi.disconnect();
    connected = false;
    connecting = false;
}

bool WiFiModule::isConnected() {
    if (connecting) {
        connected = (WiFi.status() == WL_CONNECTED);
        if (connected) {
            connecting = false;
        }
    }
    return connected;
}

IPAddress WiFiModule::getIP() {
    return WiFi.localIP();
}