#ifndef LOGGING_H
#define LOGGING_H

#include "esp_log.h"

enum class LogModule {
    NetworkController,
    WiFiModule,
    MQTTModule,
    Main,
    ButtonModule,
    SoilMoistureModule,
    DHTModule,
    ConfigLoader
};

namespace Logger {
    const char* getTag(LogModule module);
    void setModuleLevel(LogModule module, esp_log_level_t level);
    void setDefaultLevel(esp_log_level_t level);
}

#define LOGV(module, format, ...) ESP_LOGV(Logger::getTag(module), format, ##__VA_ARGS__)
#define LOGD(module, format, ...) ESP_LOGD(Logger::getTag(module), format, ##__VA_ARGS__)
#define LOGI(module, format, ...) ESP_LOGI(Logger::getTag(module), format, ##__VA_ARGS__)
#define LOGW(module, format, ...) ESP_LOGW(Logger::getTag(module), format, ##__VA_ARGS__)
#define LOGE(module, format, ...) ESP_LOGE(Logger::getTag(module), format, ##__VA_ARGS__)

#endif // LOGGING_H
