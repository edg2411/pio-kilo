#include "Logging.h"

namespace {
    constexpr const char* TAG_NETWORK = "NetworkController";
    constexpr const char* TAG_WIFI = "WiFiModule";
    constexpr const char* TAG_MQTT = "MQTTModule";
    constexpr const char* TAG_MAIN = "Main";
    constexpr const char* TAG_BUTTON = "ButtonModule";
    constexpr const char* TAG_SOIL = "SoilMoistureModule";
    constexpr const char* TAG_DHT = "DHTModule";
    constexpr const char* TAG_CONFIG = "ConfigLoader";
}

const char* Logger::getTag(LogModule module) {
    switch (module) {
        case LogModule::NetworkController:
            return TAG_NETWORK;
        case LogModule::WiFiModule:
            return TAG_WIFI;
        case LogModule::MQTTModule:
            return TAG_MQTT;
        case LogModule::Main:
            return TAG_MAIN;
        case LogModule::ButtonModule:
            return TAG_BUTTON;
        case LogModule::SoilMoistureModule:
            return TAG_SOIL;
        case LogModule::DHTModule:
            return TAG_DHT;
        case LogModule::ConfigLoader:
            return TAG_CONFIG;
        default:
            return "Unknown";
    }
}

void Logger::setModuleLevel(LogModule module, esp_log_level_t level) {
    esp_log_level_set(getTag(module), level);
}

void Logger::setDefaultLevel(esp_log_level_t level) {
    esp_log_level_set("*", level);
}
