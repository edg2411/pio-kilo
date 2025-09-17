#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <ETH.h>
#include <WiFi.h>
#include <vector>
#include <String>

class WebServerModule {
private:
    WiFiServer* server;
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
    void handleLogin(WiFiClient& client, String request);
    void handleControl(WiFiClient& client, String request);
    void handleLogout(WiFiClient& client);
    void handleRoot(WiFiClient& client, String request);
    
    // Utility
    String getContentType(String filename);
    String urlDecode(String str);
    void parseFormData(String data, String& username, String& password);

public:
    WebServerModule(int port = 80, int relayPin = 15, int ledPin = 14);
    ~WebServerModule();
    
    void begin();
    void handleClient();
    
    // Relay control
    void setRelayState(bool state);
    bool getRelayState();
};

#endif // WEB_SERVER_MODULE_H