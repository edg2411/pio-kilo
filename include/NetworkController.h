#ifndef NETWORK_CONTROLLER_H
#define NETWORK_CONTROLLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ETH.h>
#include <vector>

enum NetInterface {
    ETHERNET,
    WIFI,
    LTE
};

enum NetworkState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
};

typedef void (*NetworkEventCallback)(NetInterface interface);

class WiFiModule;
class EthernetModule;
class LTEModule;

class NetworkController {
private:
    NetInterface currentInterface;
    NetworkState state;
    NetworkEventCallback onConnectedCallback;
    NetworkEventCallback onDisconnectedCallback;

    WiFiModule* wifi;
    EthernetModule* ethernet;
    LTEModule* lte;

    std::vector<NetInterface> priorityOrder = {ETHERNET};
    int retryCount;
    const int maxRetries = 3;
    unsigned long lastRetryTime;
    const unsigned long retryDelay = 5000; // 5 seconds

    static NetworkController* instance;

    void attemptConnection(NetInterface interface);
    void checkConnection();
    void triggerFailover();

    static void networkEventHandler(arduino_event_id_t event, arduino_event_info_t info);

public:
    NetworkController();
    ~NetworkController();

    void begin();
    void update();

    void setOnConnectedCallback(NetworkEventCallback cb);
    void setOnDisconnectedCallback(NetworkEventCallback cb);

    void setWiFiCredentials(const String& ssid, const String& password);
    void setWiFiStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    void setEthernetConfig(byte mac[6], IPAddress ip, IPAddress gateway, IPAddress subnet);
    void setEthernetStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    void setLTEAPN(const String& apn, const String& user = "", const String& pass = "");

    // Network configuration
    void loadAndApplyNetworkConfig();

    NetInterface getCurrentInterface();
    NetworkState getState();
    IPAddress getIP();
};

#endif // NETWORK_CONTROLLER_H