#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <WiFi.h>

class WiFiModule {
private:
    String ssid;
    String password;
    bool connected;
    bool connecting;
    bool useStaticIP;
    IPAddress staticIP;
    IPAddress staticGateway;
    IPAddress staticSubnet;
    IPAddress staticDNS1;
    IPAddress staticDNS2;

public:
    WiFiModule();
    void setCredentials(const String& ssid, const String& password);
    void setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    void enableStaticIP(bool enable);
    bool connect();
    void disconnect();
    bool isConnected();
    IPAddress getIP();
};

#endif // WIFI_MODULE_H