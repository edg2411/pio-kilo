#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <ETH.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <time.h>
#include <sys/time.h>
#include <vector>
#include <String>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct LogEntry {
    String timestamp;
    String action;
};

class WebServerModule {
private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    std::vector<LogEntry> logs;
    const char* LOGS_FILE = "/logs.json";
    int relayPin;
    int ledPin;
    bool relayState;
    String sessionToken;

    // Configurable user credentials
    String USERNAME = "admin";
    String PASSWORD = "admin";

    // Hardcoded admin credentials (fallback)
    const String ADMIN_USERNAME = "admin";
    const String ADMIN_PASSWORD = "admin123";
    
    // HTML templates
    String getLoginPage(bool error = false);
    String getControlPage();
    String getLogsPage();
    String getConfigPage();
    String getRestartPage();
    String getLogsHTML();
    String getAllLogsHTML();
    String getHeader();
    String getFooter();
    
    // Authentication
    bool authenticate(String username, String password);
    String generateSessionToken();
    bool validateSession(String token);
    
    // Request handling
    void handleRoot(AsyncWebServerRequest *request);
    void handleLogin(AsyncWebServerRequest *request);
    void handleControl(AsyncWebServerRequest *request);
    void handleLogsPage(AsyncWebServerRequest *request);
    void handleConfigPage(AsyncWebServerRequest *request);
    void handleConfigUpdate(AsyncWebServerRequest *request);
    void handleLogout(AsyncWebServerRequest *request);
    void handleOpen(AsyncWebServerRequest *request);
    void handleClose(AsyncWebServerRequest *request);
    
    // Utility
    String getContentType(String filename);

public:
    WebServerModule(int port = 80, int relayPin = 15, int ledPin = 14);
    ~WebServerModule();

    void begin();

    // NTP
    String getCurrentTime();

    // Logging
    void addLog(String action);
    void sendLogToClients(LogEntry log);
    void loadLogsFromFile();
    void saveLogsToFile();

    // Configuration
    void loadUserConfig();
    void saveUserConfig();
    void loadNetworkConfig();
    void saveNetworkConfig(bool dhcp, String ip, String gateway, String subnet, String dns1);

    // WebSocket
    void sendButtonEvent();

    // Relay control
    void setRelayState(bool state);
    bool getRelayState();
};

#endif // WEB_SERVER_MODULE_H