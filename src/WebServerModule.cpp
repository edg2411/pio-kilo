#include "WebServerModule.h"
#include <Arduino.h>

WebServerModule::WebServerModule(int port, int relayPin, int ledPin) : relayState(false) {
    this->relayPin = relayPin;
    this->ledPin = ledPin;
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");
    sessionToken = "";
}

WebServerModule::~WebServerModule() {
    delete server;
    delete ws;
}

void WebServerModule::begin() {
    // Register routes
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });

    server->on("/login", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleLogin(request);
    });

    server->on("/control", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleControl(request);
    });

    server->on("/control", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleControl(request);
    });

    server->on("/logout", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLogout(request);
    });

    server->on("/open", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleOpen(request);
    });

    server->on("/close", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleClose(request);
    });

    // WebSocket setup
    ws->onEvent([this](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
        // WebSocket event handling - no logging to reduce noise
    });
    server->addHandler(ws);

    server->begin();
    Serial.println("Async web server started");

    // Setup NTP with Argentina timezone
    configTzTime("ART3", "pool.ntp.org");
    Serial.println("NTP configured");
}


void WebServerModule::handleRoot(AsyncWebServerRequest *request) {
    // Check if user is already logged in
    if (request->hasParam("session") && validateSession(request->getParam("session")->value())) {
        // Redirect to control page
        request->redirect("/control?session=" + request->getParam("session")->value());
        return;
    }

    // Show login page
    request->send(200, "text/html", getLoginPage());
}

void WebServerModule::handleLogin(AsyncWebServerRequest *request) {
    String username = "", password = "";
    if (request->hasParam("username", true)) {
        username = request->getParam("username", true)->value();
    }
    if (request->hasParam("password", true)) {
        password = request->getParam("password", true)->value();
    }

    if (authenticate(username, password)) {
        sessionToken = generateSessionToken();
        Serial.println("Login successful");
        // Redirect to control page with session
        request->redirect("/control?session=" + sessionToken);
    } else {
        Serial.println("Login failed - invalid credentials");
        // Show login page with error
        request->send(200, "text/html", getLoginPage(true));
    }
}

void WebServerModule::handleControl(AsyncWebServerRequest *request) {
    // Reduced logging to prevent Serial buffer issues
    // Check authentication
    bool isAuthenticated = false;
    String sessionParam = "";

    if (request->method() == HTTP_GET) {
        if (request->hasParam("session")) {
            sessionParam = request->getParam("session")->value();
        }
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    } else if (request->method() == HTTP_POST) {
        if (request->hasParam("session", true)) {
            sessionParam = request->getParam("session", true)->value();
        }
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    // Handle POST requests for relay control
    if (request->method() == HTTP_POST) {
        if (request->hasParam("action", true)) {
            String action = request->getParam("action", true)->value();
            if (action == "open") {
                setRelayState(true);
            } else if (action == "close") {
                setRelayState(false);
            } else if (action == "toggle") {
                setRelayState(!getRelayState());
            }
        }
    }
    request->send(200, "text/html", getControlPage());
}

void WebServerModule::handleLogout(AsyncWebServerRequest *request) {
    sessionToken = "";
    request->redirect("/");
}

void WebServerModule::handleOpen(AsyncWebServerRequest *request) {
    setRelayState(true);
    request->redirect("/control");
}

void WebServerModule::handleClose(AsyncWebServerRequest *request) {
    setRelayState(false);
    request->redirect("/control");
}

String WebServerModule::getLoginPage(bool error) {
    String html = getHeader();
    html += "<div class='login-container'>";
    html += "<h1>Control de acceso</h1>";
    html += "<h2>Sucursal 001</h2>";
    
    if (error) {
        html += "<div class='error'>Invalid username or password</div>";
    }
    
    html += "<form method='POST' action='/login'>";
    html += "<div class='form-group'>";
    html += "<label for='username'>Usuario:</label>";
    html += "<input type='text' id='username' name='username' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='password'>Password:</label>";
    html += "<input type='password' id='password' name='password' required>";
    html += "</div>";
    html += "<button type='submit' class='btn btn-primary'>Ingresar</button>";
    html += "</form>";
    html += "</div>";
    html += getFooter();
    return html;
}

String WebServerModule::getControlPage() {
    // CRITICAL: Only read hardware state - NEVER write to it
    bool actualHardwareState = digitalRead(relayPin);

    // Update internal state to match hardware for UI display
    relayState = actualHardwareState;

    String html = getHeader();
    html += "<div class='control-container'>";
    html += "<h1>Control de acceso</h1>";
    html += "<div class='status'>";
    html += "<h2>Estado: <span class='" + String(relayState ? "status-open" : "status-closed") + "'>" + String(relayState ? "ABIERTO" : "CERRADO") + "</span></h2>";
    html += "</div>";
    
    html += "<div class='controls'>";
    html += "<form method='POST' action='/control' style='display: inline;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='action' value='open'>";
    html += "<button type='submit' class='btn btn-success'>Abrir</button>";
    html += "</form>";

    html += "<form method='POST' action='/control' style='display: inline; margin-left: 20px;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='action' value='close'>";
    html += "<button type='submit' class='btn btn-danger'>Cerrar</button>";
    html += "</form>";
    html += "</div>";

    html += "<div class='logs'>";
    html += "<h2>Historial</h2>";
    html += getLogsHTML();
    html += "</div>";

    html += "<div class='logout'>";
    html += "<a href='/logout' class='btn btn-secondary'>Salir</a>";
    html += "</div>";
    html += "</div>";

    // WebSocket client code
    html += "<script>";
    html += "var ws = new WebSocket('ws://' + window.location.host + '/ws');";
    html += "ws.onopen = function() { console.log('WebSocket connected'); };";
    html += "ws.onmessage = function(event) {";
    html += "    if (event.data === 'button_pressed') {";
    html += "        var sessionValue = document.querySelector('input[name=\"session\"]').value;";
    html += "        var form = document.createElement('form');";
    html += "        form.method = 'POST';";
    html += "        form.action = '/control';";
    html += "        var sessionInput = document.createElement('input');";
    html += "        sessionInput.type = 'hidden';";
    html += "        sessionInput.name = 'session';";
    html += "        sessionInput.value = sessionValue;";
    html += "        var actionInput = document.createElement('input');";
    html += "        actionInput.type = 'hidden';";
    html += "        actionInput.name = 'action';";
    html += "        actionInput.value = 'toggle';";
    html += "        form.appendChild(sessionInput);";
    html += "        form.appendChild(actionInput);";
    html += "        document.body.appendChild(form);";
    html += "        form.submit();";
    html += "    } else if (event.data.startsWith('log:')) {";
    html += "        var parts = event.data.substring(4).split(',');";
    html += "        var timestamp = parts[0];";
    html += "        var action = parts[1];";
    html += "        var table = document.getElementById('logTable').getElementsByTagName('tbody')[0];";
    html += "        var row = table.insertRow(0);";
    html += "        var cell1 = row.insertCell(0);";
    html += "        var cell2 = row.insertCell(1);";
    html += "        cell1.innerHTML = timestamp;";
    html += "        cell2.innerHTML = action;";
    html += "    }";
    html += "};";
    html += "ws.onclose = function() { console.log('WebSocket disconnected'); };";
    html += "</script>";

    html += getFooter();
    return html;
}

String WebServerModule::getLogsHTML() {
    String html = "<table id='logTable'><thead><tr><th>Fecha y Hora</th><th>Accion</th></tr></thead><tbody>";
    for (auto it = logs.rbegin(); it != logs.rend(); ++it) {
        html += "<tr><td>" + it->timestamp + "</td><td>" + it->action + "</td></tr>";
    }
    html += "</tbody></table>";
    return html;
}

String WebServerModule::getHeader() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Control de Acceso</title>";
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
    html += ".logs{margin-top:30px;}";
    html += "table{width:100%;border-collapse:collapse;}";
    html += "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html += "th{background-color:#f2f2f2;}";
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



String WebServerModule::getCurrentTime() {
    time_t now = time(nullptr);
    struct tm ti;
    if (!localtime_r(&now, &ti)) {
        return "Time not available";
    }
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
    return String(buf);
}

void WebServerModule::addLog(String action) {
    String timestamp = getCurrentTime();
    LogEntry log = {timestamp, action};
    logs.push_back(log);
    if (logs.size() > 100) {
        logs.erase(logs.begin());
    }
    sendLogToClients(log);
}

void WebServerModule::sendLogToClients(LogEntry log) {
    String message = "log:" + log.timestamp + "," + log.action;
    ws->textAll(message);
}

void WebServerModule::sendButtonEvent() {
    ws->textAll("button_pressed");
}

void WebServerModule::setRelayState(bool state) {
    relayState = state;
    digitalWrite(relayPin, state ? HIGH : LOW);
    digitalWrite(ledPin, state ? HIGH : LOW);
    // Log the action
    String action = state ? "ABRIR" : "CERRAR";
    addLog(action);
}

bool WebServerModule::getRelayState() {
    return relayState;
}