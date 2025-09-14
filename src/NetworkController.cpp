#include "NetworkController.h"
#include "WiFiModule.h"
#include "EthernetModule.h"
#include "LTEModule.h"
#include "board.h"

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
    Serial.println("LTE hardware initialization skipped");

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
    } else if (state == CONNECTED) {
        // Check for higher priority interfaces that are available
        size_t currentIndex = 0;
        for (size_t i = 0; i < priorityOrder.size(); i++) {
            if (priorityOrder[i] == currentInterface) {
                currentIndex = i;
                break;
            }
        }
        for (size_t i = 0; i < currentIndex; i++) {
            NetInterface higher = priorityOrder[i];
            bool isHigherConnected = false;
            switch (higher) {
                case ETHERNET:
                    isHigherConnected = ethernet->isConnected();
                    break;
                case WIFI:
                    isHigherConnected = wifi->isConnected();
                    break;
                case LTE:
                    isHigherConnected = lte && lte->isConnected();
                    break;
            }
            if (isHigherConnected) {
                // Switch to higher priority interface
                currentInterface = higher;
                if (onConnectedCallback) onConnectedCallback(higher);
                break; // Switch to the highest available
            }
        }
    }
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

void NetworkController::attemptConnection(NetInterface interface) {
    state = CONNECTING;
    bool success = false;

    switch (interface) {
        case ETHERNET:
            success = ethernet->connect();
            break;
        case WIFI:
            success = wifi->connect();
            break;
        case LTE:
            success = lte && lte->connect();
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
        case WIFI:
            isConnected = wifi->isConnected();
            break;
        case LTE:
            isConnected = lte && lte->isConnected();
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

void NetworkController::setLTEAPN(const String& apn, const String& user, const String& pass) {
    if (lte) {
        lte->setAPN(apn, user, pass);
    } else {
        Serial.println("LTE not available, skipping APN setup");
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