#include "NetworkController.h"
#include "WiFiModule.h"
#include "EthernetModule.h"
#include "LTEModule.h"
#include "board.h"
#include "ConfigLoader.h"
#include <ESPmDNS.h>

NetworkController* NetworkController::instance = nullptr;

NetworkController::NetworkController() :
    currentInterface(ETHERNET),
    state(DISCONNECTED),
    onConnectedCallback(nullptr),
    onDisconnectedCallback(nullptr),
    wifi(new WiFiModule()),
    ethernet(new EthernetModule()),
    lte(nullptr), // Initialize later with serial
    retryCount(0),
    lastRetryTime(0)
{
}

NetworkController::~NetworkController() {
    delete wifi;
    delete ethernet;
    delete lte;
}

void NetworkController::begin() {
    instance = this;
    WiFi.onEvent(networkEventHandler);

    // Skip LTE initialization since this ESP32 has no LTE hardware
    // HardwareSerial* lteSerial = new HardwareSerial(LTE_SERIAL_NUM);
    // lteSerial->begin(LTE_SERIAL_BAUD, SERIAL_8N1, LTE_RX_PIN, LTE_TX_PIN);
    // lte = new LTEModule(lteSerial);
    lte = nullptr;  // Set to null to prevent crashes

    // Set default credentials (user should set via methods)
    // wifi->setCredentials("SSID", "PASS");
    // ethernet->setConfig(...);
    // lte->setAPN("APN");

    // Start with first priority
    attemptConnection(priorityOrder[0]);
}

void NetworkController::update() {
    checkConnection();

    if (state == DISCONNECTED) {
        if (millis() - lastRetryTime > retryDelay) {
            triggerFailover();
        }
    }
    // No need to check for higher priority since only Ethernet is used
}

void NetworkController::setOnConnectedCallback(NetworkEventCallback cb) {
    onConnectedCallback = cb;
}

void NetworkController::setOnDisconnectedCallback(NetworkEventCallback cb) {
    onDisconnectedCallback = cb;
}

NetInterface NetworkController::getCurrentInterface() {
    return currentInterface;
}

NetworkState NetworkController::getState() {
    return state;
}

IPAddress NetworkController::getIP() {
    if (currentInterface == ETHERNET && ethernet) {
        return ethernet->getIP();
    }
    return IPAddress(0, 0, 0, 0);
}

void NetworkController::attemptConnection(NetInterface interface) {
    state = CONNECTING;
    bool success = false;

    switch (interface) {
        case ETHERNET:
            success = ethernet->connect();
            break;
        default:
            // Only Ethernet is supported
            success = false;
            break;
    }

    if (success) {
        state = CONNECTED;
        currentInterface = interface;
        if (onConnectedCallback) onConnectedCallback(interface);
    } else {
        state = DISCONNECTED;
    }
}

void NetworkController::checkConnection() {
    bool isConnected = false;

    switch (currentInterface) {
        case ETHERNET:
            isConnected = ethernet->isConnected();
            break;
        default:
            // Only Ethernet is supported
            isConnected = false;
            break;
    }

    if (state == CONNECTED && !isConnected) {
        state = DISCONNECTED;
        if (onDisconnectedCallback) onDisconnectedCallback(currentInterface);
        lastRetryTime = millis();
        retryCount = 0;
    }
}

void NetworkController::triggerFailover() {
    if (retryCount < maxRetries) {
        retryCount++;
        attemptConnection(currentInterface);
    } else {
        // Switch to next interface
        size_t currentIndex = 0;
        for (size_t i = 0; i < priorityOrder.size(); i++) {
            if (priorityOrder[i] == currentInterface) {
                currentIndex = i;
                break;
            }
        }
        size_t nextIndex = (currentIndex + 1) % priorityOrder.size();
        currentInterface = priorityOrder[nextIndex];
        retryCount = 0;
        attemptConnection(currentInterface);
    }
}

void NetworkController::setWiFiCredentials(const String& ssid, const String& password) {
    wifi->setCredentials(ssid, password);
}

void NetworkController::setWiFiStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
    wifi->setStaticIP(ip, gateway, subnet, dns1, dns2);
    wifi->enableStaticIP(true);
}

void NetworkController::setEthernetConfig(byte mac[6], IPAddress ip, IPAddress gateway, IPAddress subnet) {
    ethernet->setConfig(mac, ip, gateway, subnet);
}

void NetworkController::setEthernetStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2) {
    ethernet->setStaticIP(ip, gateway, subnet, dns1, dns2);
    ethernet->enableStaticIP(true);
}

void NetworkController::setLTEAPN(const String& apn, const String& user, const String& pass) {
    if (lte) {
        lte->setAPN(apn, user, pass);
    } else {
        Serial.println("LTE not available, skipping APN setup");
    }
}

static bool mdnsStarted = false;

static void setupMDNS(const char* hostname) {
    // Close any existing mDNS
    MDNS.end();

    // Try to start mDNS with our hostname
    if (MDNS.begin(hostname)) {
        Serial.print("mDNS responder started - device accessible at ");
        Serial.print(hostname);
        Serial.println(".local");

        // Remove service to avoid potential conflicts with ArduinoOTA
        // MDNS.addService("http", "tcp", 80);
        mdnsStarted = true;
    } else {
        Serial.println("Failed to start mDNS responder");
        mdnsStarted = false;
    }
}

void NetworkController::networkEventHandler(arduino_event_id_t event, arduino_event_info_t info) {
    if (!instance) return;

    switch (event) {
        case ARDUINO_EVENT_ETH_CONNECTED:
            instance->state = CONNECTED;
            instance->currentInterface = ETHERNET;
            if (instance->onConnectedCallback) instance->onConnectedCallback(ETHERNET);
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
        {
            Serial.println("Ethernet IP assigned: " + ETH.localIP().toString());
            byte mac[6];
            ETH.macAddress(mac);
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            Serial.println("Ethernet MAC: " + String(macStr));
            // Setup mDNS
            String hostname = ConfigLoader::getEthernetHostname();
            setupMDNS(hostname.c_str());
            break;
        }
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            instance->state = DISCONNECTED;
            if (instance->onDisconnectedCallback) instance->onDisconnectedCallback(ETHERNET);
            if (instance->currentInterface == ETHERNET) {
                instance->lastRetryTime = millis();
                instance->retryCount = 0;
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            instance->state = CONNECTED;
            instance->currentInterface = WIFI;
            if (instance->onConnectedCallback) instance->onConnectedCallback(WIFI);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            instance->state = DISCONNECTED;
            if (instance->onDisconnectedCallback) instance->onDisconnectedCallback(WIFI);
            if (instance->currentInterface == WIFI) {
                instance->lastRetryTime = millis();
                instance->retryCount = 0;
            }
            break;
        case ARDUINO_EVENT_PPP_CONNECTED:
            instance->state = CONNECTED;
            instance->currentInterface = LTE;
            if (instance->onConnectedCallback) instance->onConnectedCallback(LTE);
            break;
        case ARDUINO_EVENT_PPP_DISCONNECTED:
            instance->state = DISCONNECTED;
            if (instance->onDisconnectedCallback) instance->onDisconnectedCallback(LTE);
            if (instance->currentInterface == LTE) {
                instance->lastRetryTime = millis();
                instance->retryCount = 0;
            }
            break;
        default:
            break;
    }
}