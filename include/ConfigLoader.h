#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <LittleFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class ConfigLoader {
private:
    static JsonDocument config;

public:
    static bool loadConfig();
    static String getWiFiSSID();
    static String getWiFiPassword();
    static String getMQTTBroker();
    static int getMQTTPort();
    static String getMQTTClientId();
    static String getMQTTUsername();
    static String getMQTTPassword();
    static void getEthernetMAC(byte mac[6]);
    static IPAddress getEthernetIP();
    static IPAddress getEthernetGateway();
    static IPAddress getEthernetSubnet();

    static bool getEthernetStaticIPEnabled();
    static IPAddress getEthernetStaticIP();
    static IPAddress getEthernetStaticGateway();
    static IPAddress getEthernetStaticSubnet();
    static IPAddress getEthernetStaticDNS1();
    static IPAddress getEthernetStaticDNS2();
    static String getEthernetHostname();

    static String getLTEAPN();
    static String getLTEUser();
    static String getLTEPass();

    static bool getWiFiStaticIPEnabled();
    static IPAddress getWiFiStaticIP();
    static IPAddress getWiFiStaticGateway();
    static IPAddress getWiFiStaticSubnet();
    static IPAddress getWiFiStaticDNS1();
    static IPAddress getWiFiStaticDNS2();

    static String getMQTTStatusTopic();
    static String getMQTTCommandTopic();
    static String getMQTTSensorTopic();
    static String getMQTTHeartbeatTopic();

    static String getCACertFilename();
    static String getClientCertFilename();
    static String getPrivateKeyFilename();

    static String loadCACert();
    static String loadClientCert();
    static String loadPrivateKey();
};

#endif // CONFIG_LOADER_H