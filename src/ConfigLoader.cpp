#include "ConfigLoader.h"

JsonDocument ConfigLoader::config;

bool ConfigLoader::loadConfig() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Failed to open config.json");
        return false;
    }
    DeserializationError error = deserializeJson(config, file);
    file.close();
    if (error) {
        Serial.println("Failed to parse config.json");
        return false;
    }
    return true;
}

String ConfigLoader::getWiFiSSID() {
    return config["wifi"]["ssid"] | "";
}

String ConfigLoader::getWiFiPassword() {
    return config["wifi"]["password"] | "";
}

String ConfigLoader::getMQTTBroker() {
    return config["mqtt"]["broker"] | "";
}

int ConfigLoader::getMQTTPort() {
    return config["mqtt"]["port"] | 8883;
}

String ConfigLoader::getMQTTClientId() {
    return config["mqtt"]["clientId"] | "";
}

String ConfigLoader::getMQTTUsername() {
    return config["mqtt"]["username"] | "";
}

String ConfigLoader::getMQTTPassword() {
    return config["mqtt"]["password"] | "";
}

void ConfigLoader::getEthernetMAC(byte mac[6]) {
    String macStr = config["ethernet"]["mac"] | "DE:AD:BE:EF:FE:ED";
    sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

IPAddress ConfigLoader::getEthernetIP() {
    String ipStr = config["ethernet"]["staticIP"]["ip"] | "192.168.1.100";
    return IPAddress(ipStr.c_str());
}

IPAddress ConfigLoader::getEthernetGateway() {
    String gwStr = config["ethernet"]["staticIP"]["gateway"] | "192.168.1.1";
    return IPAddress(gwStr.c_str());
}

IPAddress ConfigLoader::getEthernetSubnet() {
    String snStr = config["ethernet"]["staticIP"]["subnet"] | "255.255.255.0";
    return IPAddress(snStr.c_str());
}

bool ConfigLoader::getEthernetStaticIPEnabled() {
    return config["ethernet"]["staticIP"]["enabled"] | false;
}

IPAddress ConfigLoader::getEthernetStaticIP() {
    String ipStr = config["ethernet"]["staticIP"]["ip"] | "192.168.1.100";
    return IPAddress(ipStr.c_str());
}

IPAddress ConfigLoader::getEthernetStaticGateway() {
    String gwStr = config["ethernet"]["staticIP"]["gateway"] | "192.168.1.1";
    return IPAddress(gwStr.c_str());
}

IPAddress ConfigLoader::getEthernetStaticSubnet() {
    String snStr = config["ethernet"]["staticIP"]["subnet"] | "255.255.255.0";
    return IPAddress(snStr.c_str());
}

IPAddress ConfigLoader::getEthernetStaticDNS1() {
    String dns1Str = config["ethernet"]["staticIP"]["dns1"] | "8.8.8.8";
    return IPAddress(dns1Str.c_str());
}

IPAddress ConfigLoader::getEthernetStaticDNS2() {
    String dns2Str = config["ethernet"]["staticIP"]["dns2"] | "8.8.4.4";
    return IPAddress(dns2Str.c_str());
}

String ConfigLoader::getLTEAPN() {
    return config["lte"]["apn"] | "";
}

String ConfigLoader::getLTEUser() {
    return config["lte"]["user"] | "";
}

String ConfigLoader::getLTEPass() {
    return config["lte"]["pass"] | "";
}

String ConfigLoader::getCACertFilename() {
    return config["certs"]["caCert"] | "ca.pem";
}

String ConfigLoader::getClientCertFilename() {
    return config["certs"]["clientCert"] | "client.pem";
}

String ConfigLoader::getPrivateKeyFilename() {
    return config["certs"]["privateKey"] | "private.key";
}

bool ConfigLoader::getWiFiStaticIPEnabled() {
    return config["wifi"]["staticIP"]["enabled"] | false;
}

IPAddress ConfigLoader::getWiFiStaticIP() {
    String ipStr = config["wifi"]["staticIP"]["ip"] | "192.168.1.150";
    return IPAddress(ipStr.c_str());
}

IPAddress ConfigLoader::getWiFiStaticGateway() {
    String gwStr = config["wifi"]["staticIP"]["gateway"] | "192.168.1.1";
    return IPAddress(gwStr.c_str());
}

IPAddress ConfigLoader::getWiFiStaticSubnet() {
    String snStr = config["wifi"]["staticIP"]["subnet"] | "255.255.255.0";
    return IPAddress(snStr.c_str());
}

IPAddress ConfigLoader::getWiFiStaticDNS1() {
    String dns1Str = config["wifi"]["staticIP"]["dns1"] | "8.8.8.8";
    return IPAddress(dns1Str.c_str());
}

IPAddress ConfigLoader::getWiFiStaticDNS2() {
    String dns2Str = config["wifi"]["staticIP"]["dns2"] | "8.8.4.4";
    return IPAddress(dns2Str.c_str());
}

String ConfigLoader::getMQTTStatusTopic() {
    return config["mqtt"]["topics"]["status"] | "home/status";
}

String ConfigLoader::getMQTTCommandTopic() {
    return config["mqtt"]["topics"]["command"] | "home/command";
}

String ConfigLoader::getMQTTSensorTopic() {
    return config["mqtt"]["topics"]["sensor"] | "home/sensor";
}

String ConfigLoader::getMQTTHeartbeatTopic() {
    return config["mqtt"]["topics"]["heartbeat"] | "home/heartbeat";
}

String ConfigLoader::loadCACert() {
    if (!LittleFS.begin(false)) {  // false = don't format if mount fails
        Serial.println("LittleFS not initialized for CA cert loading");
        return "";
    }
    String filename = "/" + getCACertFilename();
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Failed to open CA cert file: ");
        Serial.println(filename);
        return "";
    }
    String cert = file.readString();
    file.close();
    return cert;
}

String ConfigLoader::loadClientCert() {
    if (!LittleFS.begin(false)) {  // false = don't format if mount fails
        Serial.println("LittleFS not initialized for client cert loading");
        return "";
    }
    String filename = "/" + getClientCertFilename();
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Failed to open client cert file: ");
        Serial.println(filename);
        return "";
    }
    String cert = file.readString();
    file.close();
    return cert;
}

String ConfigLoader::loadPrivateKey() {
    if (!LittleFS.begin(false)) {  // false = don't format if mount fails
        Serial.println("LittleFS not initialized for private key loading");
        return "";
    }
    String filename = "/" + getPrivateKeyFilename();
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.print("Failed to open private key file: ");
        Serial.println(filename);
        return "";
    }
    String key = file.readString();
    file.close();
    return key;
}