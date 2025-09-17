#include "WebServerModule.h"
#include <Arduino.h>

WebServerModule::WebServerModule(int port, int relayPin, int ledPin) : relayState(false) {
    this->relayPin = relayPin;
    this->ledPin = ledPin;
    server = new WiFiServer(port);
    sessionToken = "";
}

WebServerModule::~WebServerModule() {
    delete server;
}

void WebServerModule::begin() {
    server->begin();
    Serial.println("Web server started");
}

void WebServerModule::handleClient() {
    WiFiClient client = server->accept();
    if (client) {
        String request = "";
        bool requestComplete = false;
        unsigned long startTime = millis();

        // Read the entire request with timeout
        while (client.connected() && !requestComplete && millis() - startTime < 1000) {
            if (client.available()) {
                char c = client.read();
                request += c;

                // Check for end of HTTP request (double newline)
                if (request.endsWith("\r\n\r\n")) {
                    requestComplete = true;
                }
            }
        }

        // Debug: Print the request
        Serial.println("=== REQUEST RECEIVED ===");
        Serial.println(request);
        Serial.println("=== END REQUEST ===");

        // Filter out completely empty requests (browser keep-alive, etc.)
        if (request.length() == 0) {
            // Silently ignore empty requests
            client.stop();
            return;
        }

        // Filter out invalid requests (too short or wrong format)
        if (request.length() < 20 || !request.startsWith("GET ") && !request.startsWith("POST ")) {
            Serial.println("=== INVALID REQUEST DETECTED ===");
            Serial.println("Length: " + String(request.length()));
            Serial.println("Starts with GET: " + String(request.startsWith("GET ")));
            Serial.println("Starts with POST: " + String(request.startsWith("POST ")));
            Serial.println("Raw request: '" + request + "'");
            Serial.println("=== END INVALID REQUEST ===");

            client.println("HTTP/1.1 400 Bad Request");
            client.println("Connection: close");
            client.println();
            client.stop();
            return;
        }

        // Check if this is a POST request that needs body data
        if (request.indexOf("POST /login") >= 0 || request.indexOf("POST /control") >= 0) {
            // Extract Content-Length from headers
            int contentLength = 0;
            int clIndex = request.indexOf("Content-Length: ");
            if (clIndex >= 0) {
                int clEnd = request.indexOf("\r\n", clIndex);
                if (clEnd > clIndex) {
                    String clStr = request.substring(clIndex + 16, clEnd);
                    contentLength = clStr.toInt();
                }
            }

            // Read POST body if present
            if (contentLength > 0 && millis() - startTime < 1500) {
                String body = "";
                int bytesRead = 0;
                while (client.connected() && bytesRead < contentLength && millis() - startTime < 1500) {
                    if (client.available()) {
                        char c = client.read();
                        body += c;
                        bytesRead++;
                    }
                }
                request += body;
                Serial.println("POST Body: " + body);
            }
        }

        // Parse request
        if (request.indexOf("GET / ") >= 0 || request.indexOf("GET /login") >= 0) {
            Serial.println("Handling root/login request");
            handleRoot(client, request);
        } else if (request.indexOf("POST /login") >= 0) {
            Serial.println("Handling login POST request");
            handleLogin(client, request);
        } else if (request.indexOf("GET /control") >= 0) {
            Serial.println("Handling control GET request");
            handleControl(client, request);
        } else if (request.indexOf("POST /control") >= 0) {
            Serial.println("Handling control POST request");
            handleControl(client, request);
        } else if (request.indexOf("GET /logout") >= 0) {
            Serial.println("Handling logout request");
            handleLogout(client);
        } else if (request.indexOf("GET /open") >= 0) {
            Serial.println("Handling open request");
            // Direct relay control (for backward compatibility)
            setRelayState(true);
            client.println("HTTP/1.1 302 Found");
            client.println("Location: /control");
            client.println("Connection: close");
            client.println();
        } else if (request.indexOf("GET /close") >= 0) {
            Serial.println("Handling close request");
            // Direct relay control (for backward compatibility)
            setRelayState(false);
            client.println("HTTP/1.1 302 Found");
            client.println("Location: /control");
            client.println("Connection: close");
            client.println();
        } else {
            Serial.println("404 - Request not recognized");
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            client.println("<h1>404 Not Found</h1>");
        }

        client.stop();
    }
}

void WebServerModule::handleRoot(WiFiClient& client, String request) {
    // Check if user is already logged in
    if (request.indexOf("session=" + sessionToken) >= 0 && validateSession(sessionToken)) {
        // Redirect to control page
        client.println("HTTP/1.1 302 Found");
        client.println("Location: /control");
        client.println("Connection: close");
        client.println();
        return;
    }
    
    // Show login page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(getLoginPage());
}

void WebServerModule::handleLogin(WiFiClient& client, String request) {
    String username, password;
    parseFormData(request, username, password);

    if (authenticate(username, password)) {
        sessionToken = generateSessionToken();
        Serial.println("Login successful");
        // Redirect to control page with session
        client.println("HTTP/1.1 302 Found");
        client.println("Location: /control?session=" + sessionToken);
        client.println("Connection: close");
        client.println();
    } else {
        Serial.println("Login failed - invalid credentials");
        // Show login page with error
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println(getLoginPage(true));
    }
}

void WebServerModule::handleControl(WiFiClient& client, String request) {
    Serial.println("=== CONTROL REQUEST ===");
    Serial.println("Session token: " + sessionToken);
    Serial.println("Full request: " + request);

    // Check authentication - handle both GET and POST
    bool isAuthenticated = false;

    if (request.indexOf("GET /control") >= 0) {
        // For GET requests, check URL parameters
        String sessionParam = "";
        int sessionIndex = request.indexOf("session=");
        if (sessionIndex >= 0) {
            int sessionEnd = request.indexOf(" ", sessionIndex);
            if (sessionEnd < 0) sessionEnd = request.indexOf("\r\n", sessionIndex);
            if (sessionEnd > sessionIndex) {
                sessionParam = request.substring(sessionIndex + 8, sessionEnd);
            }
        }
        Serial.println("GET session param: '" + sessionParam + "'");
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    } else if (request.indexOf("POST /control") >= 0) {
        // For POST requests, check form data
        int bodyStart = request.indexOf("\r\n\r\n");
        if (bodyStart > 0) {
            String body = request.substring(bodyStart + 4);
            Serial.println("POST body: " + body);
            if (body.indexOf("session=" + sessionToken) >= 0 && validateSession(sessionToken)) {
                isAuthenticated = true;
            }
        }
    }

    Serial.println("Authentication result: " + String(isAuthenticated ? "SUCCESS" : "FAILED"));

    if (!isAuthenticated) {
        Serial.println("Redirecting to login");
        client.println("HTTP/1.1 302 Found");
        client.println("Location: /");
        client.println("Connection: close");
        client.println();
        return;
    }

    // Handle POST requests for relay control
    if (request.indexOf("POST /control") >= 0) {
        if (request.indexOf("action=open") >= 0) {
            Serial.println("Opening door");
            setRelayState(true);
        } else if (request.indexOf("action=close") >= 0) {
            Serial.println("Closing door");
            setRelayState(false);
        }
    }

    // Show control page
    Serial.println("Serving control page");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println(getControlPage());
}

void WebServerModule::handleLogout(WiFiClient& client) {
    sessionToken = "";
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /");
    client.println("Connection: close");
    client.println();
}

String WebServerModule::getLoginPage(bool error) {
    String html = getHeader();
    html += "<div class='login-container'>";
    html += "<h1>ESP32 Door Control</h1>";
    html += "<h2>Login</h2>";
    
    if (error) {
        html += "<div class='error'>Invalid username or password</div>";
    }
    
    html += "<form method='POST' action='/login'>";
    html += "<div class='form-group'>";
    html += "<label for='username'>Username:</label>";
    html += "<input type='text' id='username' name='username' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='password'>Password:</label>";
    html += "<input type='password' id='password' name='password' required>";
    html += "</div>";
    html += "<button type='submit' class='btn btn-primary'>Login</button>";
    html += "</form>";
    html += "</div>";
    html += getFooter();
    return html;
}

String WebServerModule::getControlPage() {
    // CRITICAL: Only read hardware state - NEVER write to it
    bool actualHardwareState = digitalRead(relayPin);
    Serial.println("[PAGE LOAD] Hardware read: PIN" + String(relayPin) + "=" + String(actualHardwareState ? "HIGH" : "LOW") +
                  ", Previous internal state: " + String(relayState ? "OPEN" : "CLOSED"));

    // Update internal state to match hardware for UI display
    relayState = actualHardwareState;
    Serial.println("[PAGE LOAD] Updated UI to show: " + String(relayState ? "OPEN" : "CLOSED"));

    String html = getHeader();
    html += "<div class='control-container'>";
    html += "<h1>ESP32 Door Control</h1>";
    html += "<div class='status'>";
    html += "<h2>Door Status: <span class='" + String(relayState ? "status-open" : "status-closed") + "'>" + String(relayState ? "OPEN" : "CLOSED") + "</span></h2>";
    html += "<button onclick='location.reload()' class='btn btn-secondary' style='font-size: 12px; padding: 5px 10px; margin-left: 10px;'>Refresh</button>";
    html += "</div>";
    
    html += "<div class='controls'>";
    html += "<form method='POST' action='/control' style='display: inline;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='action' value='open'>";
    html += "<button type='submit' class='btn btn-success'>Open Door</button>";
    html += "</form>";

    html += "<form method='POST' action='/control' style='display: inline; margin-left: 20px;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='action' value='close'>";
    html += "<button type='submit' class='btn btn-danger'>Close Door</button>";
    html += "</form>";
    html += "</div>";
    
    html += "<div class='logout'>";
    html += "<a href='/logout' class='btn btn-secondary'>Logout</a>";
    html += "</div>";
    html += "</div>";
    html += getFooter();
    return html;
}

String WebServerModule::getHeader() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ESP32 Door Control</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;}";
    html += ".login-container,.control-container{max-width:400px;margin:50px auto;padding:30px;background:white;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    html += "h1{text-align:center;color:#333;margin-bottom:30px;}";
    html += "h2{margin-bottom:20px;color:#555;}";
    html += ".form-group{margin-bottom:15px;}";
    html += "label{display:block;margin-bottom:5px;font-weight:bold;}";
    html += "input[type='text'],input[type='password']{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;font-size:16px;}";
    html += ".btn{display:inline-block;padding:12px 24px;border:none;border-radius:5px;text-decoration:none;font-size:16px;cursor:pointer;margin:5px;}";
    html += ".btn-primary{background:#007bff;color:white;}";
    html += ".btn-success{background:#28a745;color:white;}";
    html += ".btn-danger{background:#dc3545;color:white;}";
    html += ".btn-secondary{background:#6c757d;color:white;}";
    html += ".status{text-align:center;margin:30px 0;}";
    html += ".status-open{color:#28a745;font-weight:bold;}";
    html += ".status-closed{color:#dc3545;font-weight:bold;}";
    html += ".controls{text-align:center;margin:30px 0;}";
    html += ".logout{text-align:center;margin-top:30px;}";
    html += ".error{color:#dc3545;background:#f8d7da;padding:10px;border-radius:5px;margin-bottom:20px;text-align:center;}";
    html += "</style></head><body>";
    return html;
}

String WebServerModule::getFooter() {
    return "</body></html>";
}

bool WebServerModule::authenticate(String username, String password) {
    return (username == USERNAME && password == PASSWORD);
}

String WebServerModule::generateSessionToken() {
    String token = "";
    for (int i = 0; i < 32; i++) {
        token += String(random(0, 16), HEX);
    }
    return token;
}

bool WebServerModule::validateSession(String token) {
    return (token == sessionToken && token.length() > 0);
}

void WebServerModule::parseFormData(String request, String& username, String& password) {
    int bodyStart = request.indexOf("\r\n\r\n");
    if (bodyStart > 0) {
        String body = request.substring(bodyStart + 4);
        int userStart = body.indexOf("username=");
        int passStart = body.indexOf("password=");
        
        if (userStart >= 0) {
            int userEnd = body.indexOf("&", userStart);
            if (userEnd < 0) userEnd = body.length();
            username = urlDecode(body.substring(userStart + 9, userEnd));
        }
        
        if (passStart >= 0) {
            int passEnd = body.indexOf("&", passStart);
            if (passEnd < 0) passEnd = body.length();
            password = urlDecode(body.substring(passStart + 9, passEnd));
        }
    }
}

String WebServerModule::urlDecode(String str) {
    str.replace("+", " ");
    str.replace("%20", " ");
    str.replace("%21", "!");
    str.replace("%22", "\"");
    str.replace("%23", "#");
    str.replace("%24", "$");
    str.replace("%25", "%");
    str.replace("%26", "&");
    str.replace("%27", "'");
    str.replace("%28", "(");
    str.replace("%29", ")");
    return str;
}

void WebServerModule::setRelayState(bool state) {
    relayState = state;
    digitalWrite(relayPin, state ? HIGH : LOW);
    digitalWrite(ledPin, state ? HIGH : LOW);
    Serial.println("Door " + String(state ? "opened" : "closed"));
}

bool WebServerModule::getRelayState() {
    return relayState;
}