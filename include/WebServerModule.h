#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <ETH.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <vector>
#include <String>

class WebServerModule {
private:
    AsyncWebServer* server;
    int relayPin;
    int ledPin;
    bool relayState;
    String sessionToken;

    // Hardcoded credentials (for now)
    const String USERNAME = "admin";
    const String PASSWORD = "admin";
    
    // HTML templates
    String getLoginPage(bool error = false);
    String getControlPage();
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
    void handleLogout(AsyncWebServerRequest *request);
    void handleOpen(AsyncWebServerRequest *request);
    void handleClose(AsyncWebServerRequest *request);
    
    // Utility
    String getContentType(String filename);

public:
    WebServerModule(int port = 80, int relayPin = 15, int ledPin = 14);
    ~WebServerModule();

    void begin();

    // Relay control
    void setRelayState(bool state);
    bool getRelayState();
};

#endif // WEB_SERVER_MODULE_H