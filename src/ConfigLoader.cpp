#include "ConfigLoader.h"
#include "Logging.h"

JsonDocument ConfigLoader::config;

bool ConfigLoader::loadConfig() {
    if (!LittleFS.begin(true)) {
        LOGE(LogModule::ConfigLoader, "LittleFS mount failed");
        return false;
    }
    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        LOGE(LogModule::ConfigLoader, "Failed to open config.json");
        return false;
    }
    DeserializationError error = deserializeJson(config, file);
    file.close();
    if (error) {
        LOGE(LogModule::ConfigLoader, "Failed to parse config.json");
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
    String ipStr = config["ethernet"]["ip"] | "192.168.1.100";
    return IPAddress(ipStr.c_str());
}

IPAddress ConfigLoader::getEthernetGateway() {
    String gwStr = config["ethernet"]["gateway"] | "192.168.1.1";
    return IPAddress(gwStr.c_str());
}

IPAddress ConfigLoader::getEthernetSubnet() {
    String snStr = config["ethernet"]["subnet"] | "255.255.255.0";
    return IPAddress(snStr.c_str());
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
    LOGE(LogModule::ConfigLoader, "LittleFS not initialized for CA cert loading");
        return "";
    }
    String filename = "/" + getCACertFilename();
    File file = LittleFS.open(filename, "r");
    if (!file) {
    LOGE(LogModule::ConfigLoader, "Failed to open CA cert file: %s", filename.c_str());
        return "";
    }
    String cert = file.readString();
    file.close();
    return cert;
}

String ConfigLoader::loadClientCert() {
    if (!LittleFS.begin(false)) {  // false = don't format if mount fails
    LOGE(LogModule::ConfigLoader, "LittleFS not initialized for client cert loading");
        return "";
    }
    String filename = "/" + getClientCertFilename();
    if (!LittleFS.exists(filename)) {
        LOGD(LogModule::ConfigLoader, "Client cert file not present (%s)", filename.c_str());
        return "";
    }
    File file = LittleFS.open(filename, "r");
    if (!file) {
        LOGE(LogModule::ConfigLoader, "Failed to open client cert file: %s", filename.c_str());
        return "";
    }
    String cert = file.readString();
    file.close();
    return cert;
}

String ConfigLoader::loadPrivateKey() {
    if (!LittleFS.begin(false)) {  // false = don't format if mount fails
    LOGE(LogModule::ConfigLoader, "LittleFS not initialized for private key loading");
        return "";
    }
    String filename = "/" + getPrivateKeyFilename();
    if (!LittleFS.exists(filename)) {
        LOGD(LogModule::ConfigLoader, "Private key file not present (%s)", filename.c_str());
        return "";
    }
    File file = LittleFS.open(filename, "r");
    if (!file) {
        LOGE(LogModule::ConfigLoader, "Failed to open private key file: %s", filename.c_str());
        return "";
    }
    String key = file.readString();
    file.close();
    return key;
}