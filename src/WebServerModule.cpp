#include "WebServerModule.h"
#include <Arduino.h>

WebServerModule::WebServerModule(int port, int relayPin, int ledPin) : relayState(false) {
    this->relayPin = relayPin;
    this->ledPin = ledPin;
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");
    sessionToken = "";
    buzzer = new BuzzerModule();

    // Initialize devices
    devices.push_back({"real", "Controlador 1", "online", "Sucursal 001"});
    devices.push_back({"mock1", "Controlador 2", "online", "Sucursal 002"});
    devices.push_back({"mock2", "Controlador 3", "offline", "Sucursal 003"});
    devices.push_back({"mock3", "Controlador 4", "online", "Sucursal 004"});

    // Initialize LittleFS and load configurations
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
    } else {
        migrateLegacyLogs(); // Migrate old logs to device-specific files
        loadDeviceLogs("real"); // Load real device logs by default
        loadUserConfig();
        Serial.printf("Loaded %d device logs and user config from file\n", currentDeviceLogs.size());
    }
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

    server->on("/dashboard", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleDashboard(request);
    });

    server->on("/control", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleControl(request);
    });

    server->on("/logout", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLogout(request);
    });

    server->on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleLogsPage(request);
    });

    server->on("/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleConfigPage(request);
    });

    server->on("/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
        handleConfigUpdate(request);
    });

    server->on("/download/logs/*", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleDownloadLogs(request);
    });

    server->on("/open", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleOpen(request);
    });

    server->on("/close", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleClose(request);
    });

    server->on("/toggle", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleToggle(request);
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

void WebServerModule::update() {
    // Update buzzer for non-blocking beeps
    buzzer->update();
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
        // Redirect to dashboard page with session
        request->redirect("/dashboard?session=" + sessionToken);
    } else {
        Serial.println("Login failed - invalid credentials");
        // Show login page with error
        request->send(200, "text/html", getLoginPage(true));
    }
}

void WebServerModule::handleDashboard(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    String sessionParam = "";

    if (request->hasParam("session")) {
        sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    request->send(200, "text/html", getDashboardPage());
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

    // Get device parameter (check both URL and POST parameters)
    String deviceId = "real"; // default
    const AsyncWebParameter* deviceParam = nullptr;

    // Check URL parameter first (for GET requests)
    if (request->hasParam("device")) {
        deviceParam = request->getParam("device");
    }
    // Check POST parameter (for POST requests)
    else if (request->hasParam("device", true)) {
        deviceParam = request->getParam("device", true);
    }

    if (deviceParam) {
        deviceId = deviceParam->value();
    }

    // Load logs for this device
    loadDeviceLogs(deviceId);

    // Handle POST requests for relay control
    if (request->method() == HTTP_POST) {
        if (request->hasParam("action", true)) {
            String action = request->getParam("action", true)->value();
            if (action == "open") {
                setRelayState(true, deviceId);
            } else if (action == "close") {
                setRelayState(false, deviceId);
            } else if (action == "toggle") {
                setRelayState(!getRelayState(), deviceId);
            }
        }
        // Reload logs after action
        loadDeviceLogs(deviceId);
    }
    request->send(200, "text/html", getControlPage(deviceId));
}

void WebServerModule::handleLogsPage(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    String sessionParam = "";

    if (request->hasParam("session")) {
        sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    // Get device parameter, default to "real"
    String selectedDevice = "real";
    if (request->hasParam("device")) {
        selectedDevice = request->getParam("device")->value();
    }

    // Load logs for the selected device
    loadDeviceLogs(selectedDevice);

    request->send(200, "text/html", getLogsPage(selectedDevice));
}

void WebServerModule::handleConfigPage(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    String sessionParam = "";

    if (request->hasParam("session")) {
        sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    request->send(200, "text/html", getConfigPage());
}

void WebServerModule::handleConfigUpdate(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    String sessionParam = "";

    if (request->hasParam("session", true)) {
        sessionParam = request->getParam("session", true)->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    String section = request->getParam("section", true)->value();

    if (section == "credentials") {
        String newUser = request->getParam("username", true)->value();
        String newPass = request->getParam("password", true)->value();
        String confirm = request->getParam("confirm_password", true)->value();

        if (newPass == confirm && newUser.length() > 0) {
            USERNAME = newUser;
            PASSWORD = newPass;
            saveUserConfig();
            request->redirect("/config?session=" + sessionToken + "&success=true");
        } else {
            request->redirect("/config?session=" + sessionToken + "&error=true");
        }
    } else if (section == "network") {
        // Handle network configuration - extract form data
        bool dhcp = (request->getParam("dhcp", true)->value() == "true");
        String ip = request->getParam("ip", true)->value();
        String gateway = request->getParam("gateway", true)->value();
        String subnet = request->getParam("subnet", true)->value();
        String dns1 = request->getParam("dns1", true)->value();

        // Save the network configuration
        saveNetworkConfig(dhcp, ip, gateway, subnet, dns1);

        // Send restart page and then reboot
        request->send(200, "text/html", getRestartPage());
        // Delay to allow response to be sent, then reboot
        delay(1000);
        ESP.restart();
    } else if (section == "datetime") {
        // Handle manual date/time configuration
        String dateStr = request->getParam("date", true)->value();
        String timeStr = request->getParam("time", true)->value();

        if (dateStr.length() > 0 && timeStr.length() > 0) {
            // Parse date and time
            int year = dateStr.substring(0, 4).toInt();
            int month = dateStr.substring(5, 7).toInt();
            int day = dateStr.substring(8, 10).toInt();
            int hour = timeStr.substring(0, 2).toInt();
            int minute = timeStr.substring(3, 5).toInt();

            // Set manual time
            struct tm tm;
            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = 0;

            time_t t = mktime(&tm);
            struct timeval tv = {t, 0};
            settimeofday(&tv, NULL);

            Serial.printf("Manual time set: %04d-%02d-%02d %02d:%02d\n", year, month, day, hour, minute);
        }

        request->redirect("/config?session=" + sessionToken + "&success=true");
    } else {
        request->redirect("/config?session=" + sessionToken);
    }
}

void WebServerModule::handleLogout(AsyncWebServerRequest *request) {
    sessionToken = "";
    request->redirect("/");
}

void WebServerModule::handleDownloadLogs(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    String sessionParam = "";

    if (request->hasParam("session")) {
        sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    // Extract device ID from URL path
    String url = request->url();
    String deviceId = "real"; // default

    // URL format: /download/logs/{deviceId}
    int logsIndex = url.indexOf("/logs/");
    if (logsIndex != -1) {
        deviceId = url.substring(logsIndex + 6); // 6 = length of "/logs/"
    }

    // Load the device logs to ensure we have the latest
    loadDeviceLogs(deviceId);

    // Get filename
    String filename = getDeviceLogFilename(deviceId);

    if (!LittleFS.exists(filename)) {
        request->send(404, "text/plain", "Log file not found");
        return;
    }

    // Read the file
    File file = LittleFS.open(filename, "r");
    if (!file) {
        request->send(500, "text/plain", "Error opening log file");
        return;
    }

    // Get file size
    size_t fileSize = file.size();
    if (fileSize > 50000) { // 50KB limit for download
        file.close();
        request->send(413, "text/plain", "Log file too large to download");
        return;
    }

    // Generate CSV content from the logs
    String csvContent = "Fecha y Hora,Dispositivo,Ubicacion,Accion\n";

    // Load the logs to get the enriched data
    loadDeviceLogs(deviceId);

    for (const auto& log : currentDeviceLogs) {
        // Find device info for CSV
        String deviceName = "Desconocido";
        String location = "N/A";
        for (const auto& device : devices) {
            if (device.id == deviceId) {
                deviceName = device.name;
                location = device.location;
                break;
            }
        }

        // Escape commas in fields if needed
        String timestamp = log.timestamp;
        String action = log.action;

        // Add to CSV (escape quotes if needed)
        csvContent += "\"" + timestamp + "\",";
        csvContent += "\"" + deviceName + "\",";
        csvContent += "\"" + location + "\",";
        csvContent += "\"" + action + "\"\n";
    }

    file.close();

    // Generate timestamp for filename
    String timestamp = getCurrentTime();
    timestamp.replace(" ", "_");
    timestamp.replace("-", "");
    timestamp.replace(":", "");

    // Set headers for CSV download
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csvContent);
    response->addHeader("Content-Disposition", "attachment; filename=\"logs_" + deviceId + "_" + timestamp + ".csv\"");
    response->addHeader("Content-Length", String(csvContent.length()));
    request->send(response);
}

void WebServerModule::handleOpen(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    if (request->hasParam("session")) {
        String sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    setRelayState(true);
    request->redirect("/control?session=" + sessionToken);
}

void WebServerModule::handleClose(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    if (request->hasParam("session")) {
        String sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    setRelayState(false);
    request->redirect("/control?session=" + sessionToken);
}

void WebServerModule::handleToggle(AsyncWebServerRequest *request) {
    // Check authentication
    bool isAuthenticated = false;
    if (request->hasParam("session")) {
        String sessionParam = request->getParam("session")->value();
        isAuthenticated = (sessionParam == sessionToken && validateSession(sessionToken));
    }

    if (!isAuthenticated) {
        request->redirect("/");
        return;
    }

    toggleRelayPulse();
    request->redirect("/control?session=" + sessionToken);
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
    html += "<label for='password'>Clave:</label>";
    html += "<input type='password' id='password' name='password' required>";
    html += "</div>";
    html += "<button type='submit' class='btn btn-primary'>Ingresar</button>";
    html += "</form>";
    html += "</div>";
    html += getFooter();
    return html;
}

String WebServerModule::getDashboardPage() {
    String html = getHeader();
    html += getNavbar();
    html += "<div class='content'>";
    html += "<div class='dashboard-container'>";
    html += "<h1>Panel de Control Principal</h1>";
    html += "<h2>Sistema de Control de Acceso</h2>";

    html += "<div class='devices-grid'>";
    for (const auto& device : devices) {
        String statusClass = (device.status == "online") ? "status-online" : "status-offline";
        String statusText = (device.status == "online") ? "Online" : "Offline";

        html += "<div class='device-card'>";
        html += "<h3>" + device.name + "</h3>";
        html += "<p class='location'>" + device.location + "</p>";
        html += "<div class='device-footer'>";
        html += "<div class='status " + statusClass + "'>" + statusText + "</div>";
        if (device.status == "online") {
            html += "<a href='/control?session=" + sessionToken + "&device=" + device.id + "' class='btn btn-primary btn-small'>Controlar</a>";
        } else {
            html += "<span class='btn btn-secondary btn-small disabled'>No disponible</span>";
        }
        html += "</div>";
        html += "</div>";
    }
    html += "</div>";
    html += "</div>";
    html += "</div>";

    // Add CSS for dashboard
    html += "<style>";
    html += ".devices-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 30px 0; }";
    html += ".device-card { border: 1px solid #ddd; border-radius: 10px; padding: 20px; background: white; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
    html += ".device-card h3 { margin-top: 0; color: #333; }";
    html += ".location { color: #666; font-size: 14px; margin: 5px 0 15px 0; }";
    html += ".device-footer { display: flex; justify-content: space-between; align-items: center; }";
    html += ".status { padding: 4px 8px; border-radius: 4px; font-weight: bold; font-size: 11px; }";
    html += ".status-online { background: #d4edda; color: #155724; }";
    html += ".status-offline { background: #f8d7da; color: #721c24; }";
    html += ".btn-small { padding: 6px 12px; font-size: 12px; }";
    html += ".disabled { opacity: 0.6; cursor: not-allowed; }";
    html += "</style>";

    html += getFooter();
    return html;
}

String WebServerModule::getControlPage(String deviceId) {
    // Find device info
    String deviceName = "Controlador Principal";
    String deviceLocation = "Sucursal 001";
    bool isRealDevice = (deviceId == "real");

    for (const auto& device : devices) {
        if (device.id == deviceId) {
            deviceName = device.name;
            deviceLocation = device.location;
            break;
        }
    }

    // CRITICAL: Only read hardware state for real device - NEVER write to it
    if (isRealDevice) {
        bool actualHardwareState = digitalRead(relayPin);
        // Update internal state to match hardware for UI display
        relayState = actualHardwareState;
    }

    String html = getHeader();
    html += getNavbar();
    html += "<div class='content'>";
    html += "<div class='control-container'>";
    html += "<h1>Control de acceso</h1>";
    html += "<h2>" + deviceLocation + "</h2>";

    if (!isRealDevice) {
        html += "<div class='demo-warning'>MODO DEMO - Control simulado</div>";
    }
    // Commented out for demo with electronic lock
    // html += "<div class='status'>";
    // html += "<h2>Estado: <span class='" + String(relayState ? "status-open" : "status-closed") + "'>" + String(relayState ? "ABIERTO" : "CERRADO") + "</span></h2>";
    // html += "</div>";

    html += "<div class='controls'>";
    html += "<form method='POST' action='/control' style='display: inline;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='device' value='" + deviceId + "'>";
    html += "<input type='hidden' name='action' value='open'>";
    html += "<button type='submit' class='btn btn-success'>ABRIR</button>";
    html += "</form>";

    html += "<form method='POST' action='/control' style='display: inline; margin-left: 20px;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='device' value='" + deviceId + "'>";
    html += "<input type='hidden' name='action' value='close'>";
    html += "<button type='submit' class='btn btn-danger'>CERRAR</button>";
    html += "</form>";

    // Commented out toggle button for magnetic lock
    // html += "<a href='/toggle?session=" + sessionToken + "' class='btn btn-success'>ABRIR</a>";
    html += "</div>";

    html += "<div class='logs'>";
    html += "<h2>Historial</h2>";
    html += getDeviceLogsHTML(deviceId);
    html += "</div>";

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

    // Add CSS for demo warning
    html += "<style>";
    html += ".demo-warning { background: #fff3cd; color: #856404; padding: 10px; border-radius: 5px; margin: 20px 0; text-align: center; font-weight: bold; border: 1px solid #ffeaa7; }";
    html += "</style>";

    html += getFooter();
    return html;
}

String WebServerModule::getLogsPage(String selectedDevice) {

    String html = getHeader();
    html += getNavbar();
    html += "<div class='content'>";
    html += "<div class='logs-container'>";
    html += "<h1>Historial Completo</h1>";

    // Device selector
    html += "<div class='device-selector'>";
    html += "<form method='GET' action='/logs' style='display: inline;'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<label for='device'>Seleccionar Dispositivo: </label>";
    html += "<select name='device' id='device' onchange='this.form.submit()'>";
    for (const auto& device : devices) {
        String selected = (device.id == selectedDevice) ? " selected" : "";
        html += "<option value='" + device.id + "'" + selected + ">" + device.name + " - " + device.location + "</option>";
    }
    html += "</select>";
    html += "</form>";
    html += "</div>";

    // Stats
    html += "<div class='stats'>";
    html += "<p>Total de registros: " + String(currentDeviceLogs.size()) + "</p>";
    String lastUpdate = "Nunca";
    if (!currentDeviceLogs.empty()) {
        String ts = currentDeviceLogs.back().timestamp;
        // Check if timestamp looks valid (not 1969/1970)
        if (ts.startsWith("1969") || ts.startsWith("1970")) {
            lastUpdate = "Esperando sincronizacion NTP";
        } else {
            lastUpdate = ts;
        }
    }
    html += "<p>Ultima actualizacion: " + lastUpdate + "</p>";
    html += "</div>";

    // Download button
    html += "<div class='download-section'>";
    html += "<a href='/download/logs/" + selectedDevice + "?session=" + sessionToken + "' class='btn btn-info'>Descargar</a>";
    html += "</div>";

    // Logs table
    html += "<div class='logs-section'>";
    html += "<h2>Historial de Comandos</h2>";
    html += getDeviceLogsHTML(selectedDevice);
    html += "</div>";

    html += "</div>";
    html += "</div>";
    html += getFooter();
    return html;
}

String WebServerModule::getLogsHTML() {
    String html = "<table id='logTable'><thead><tr><th>Fecha y Hora</th><th>Sucursal</th><th>Comando</th></tr></thead><tbody>";

    // Show last 10 logs on control page (most recent first)
    size_t startIndex = (logs.size() > 10) ? (logs.size() - 10) : 0;

    for (size_t i = logs.size(); i > startIndex; --i) {
        html += "<tr><td>" + logs[i-1].timestamp + "</td><td>001</td><td>" + logs[i-1].action + "</td></tr>";
    }

    html += "</tbody></table>";
    return html;
}

String WebServerModule::getAllLogsHTML() {
    String html = "<table><thead><tr><th>Fecha y Hora</th><th>Sucursal</th><th>Comando</th></tr></thead><tbody>";
    for (const auto& log : logs) {
        html += "<tr><td>" + log.timestamp + "</td><td>001</td><td>" + log.action + "</td></tr>";
    }
    html += "</tbody></table>";
    return html;
}

String WebServerModule::getDeviceLogsHTML(String deviceId) {
    String html = "<table><thead><tr><th>Fecha y Hora</th><th>Dispositivo</th><th>Comando</th></tr></thead><tbody>";

    // Find device info
    String deviceName = "Desconocido";
    String location = "N/A";
    for (const auto& device : devices) {
        if (device.id == deviceId) {
            deviceName = device.name;
            location = device.location;
            break;
        }
    }

    // Show last 10 logs on control page (most recent first)
    size_t startIndex = (currentDeviceLogs.size() > 10) ? (currentDeviceLogs.size() - 10) : 0;

    for (size_t i = currentDeviceLogs.size(); i > startIndex; --i) {
        const auto& log = currentDeviceLogs[i-1];
        html += "<tr><td>" + log.timestamp + "</td><td>" + deviceName + " (" + location + ")</td><td>" + log.action + "</td></tr>";
    }

    html += "</tbody></table>";
    return html;
}

String WebServerModule::getConfigPage() {
    String html = getHeader();
    html += getNavbar();
    html += "<div class='content'>";
    html += "<div class='config-container'>";
    html += "<h1>Ajustes del Sistema</h1>";

    // Success/Error messages
    if (sessionToken.length() > 0) {
        // Check for success/error parameters (simplified)
        html += "<div id='messages'></div>";
    }

    // Network Settings
    html += "<div class='config-section'>";
    html += "<h3>Ajuste de Red</h3>";
    html += "<p class='warning'>Los cambios de red requieren reinicio</p>";
    html += "<form method='POST' action='/config'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='section' value='network'>";
    html += "<label><input type='radio' name='dhcp' value='true' checked> DHCP</label><br>";
    html += "<label><input type='radio' name='dhcp' value='false'> IP Estatica</label><br>";
    html += "<div class='static-fields' style='margin-top: 10px;'>";
    html += "<input type='text' name='ip' placeholder='192.168.1.100' style='margin: 5px;'><br>";
    html += "<input type='text' name='gateway' placeholder='192.168.1.1' style='margin: 5px;'><br>";
    html += "<input type='text' name='subnet' placeholder='255.255.255.0' style='margin: 5px;'><br>";
    html += "<input type='text' name='dns1' placeholder='8.8.8.8' style='margin: 5px;'>";
    html += "</div>";
    html += "<button type='submit' class='btn btn-success' style='margin-top: 10px;'>Guardar y Reiniciar</button>";
    html += "</form>";
    html += "</div>";

    // Date/Time Configuration
    html += "<div class='config-section'>";
    html += "<h3>Ajuste de Fecha y Hora</h3>";
    html += "<p class='warning'>Ajuste manual cuando no hay acceso a internet</p>";
    html += "<form method='POST' action='/config'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='section' value='datetime'>";
    html += "<input type='date' name='date' style='margin: 5px;'><br>";
    html += "<input type='time' name='time' style='margin: 5px;'><br>";
    html += "<button type='submit' class='btn btn-success' style='margin-top: 10px;'>Actualizar Fecha/Hora</button>";
    html += "</form>";
    html += "</div>";

    // User Credentials
    html += "<div class='config-section'>";
    html += "<h3>Credenciales de Usuario</h3>";
    html += "<form method='POST' action='/config'>";
    html += "<input type='hidden' name='session' value='" + sessionToken + "'>";
    html += "<input type='hidden' name='section' value='credentials'>";
    html += "<input type='text' name='username' placeholder='Usuario' value='" + USERNAME + "' required style='margin: 5px;'><br>";
    html += "<input type='password' name='password' placeholder='Nueva clave' required style='margin: 5px;'><br>";
    html += "<input type='password' name='confirm_password' placeholder='Confirmar clave' required style='margin: 5px;'><br>";
    html += "<button type='submit' class='btn btn-success' style='margin-top: 10px;'>Actualizar Credenciales</button>";
    html += "</form>";
    html += "</div>";

    html += "</div>";
    html += getFooter();
    return html;
}

String WebServerModule::getRestartPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Reiniciando...</title>";
    html += "<meta http-equiv='refresh' content='15;url=/'>";
    html += "<style>body{font-family:Arial;text-align:center;padding:50px;} .spinner{border:4px solid #f3f3f3;border-top:4px solid #3498db;border-radius:50%;width:50px;height:50px;animation:spin 2s linear infinite;margin:20px auto;} @keyframes spin{0%{transform:rotate(0deg);}100%{transform:rotate(360deg);}}</style>";
    html += "</head><body>";
    html += "<h1>Reiniciando dispositivo...</h1>";
    html += "<div class='spinner'></div>";
    html += "<p>Los cambios de configuracion de red se aplicaran despues del reinicio.</p>";
    html += "<p>Seras redirigido automaticamente en 15 segundos...</p>";
    html += "<p><a href='/'>O haz clic aqui si no se redirige</a></p>";
    html += "</body></html>";
    return html;
}

String WebServerModule::getNavbar() {
    String html = "<div class='navbar'>";
    html += "<a href='/dashboard?session=" + sessionToken + "' class='nav-btn'>Dashboard</a>";
    html += "<a href='/logs?session=" + sessionToken + "' class='nav-btn'>Historial</a>";
    html += "<a href='/config?session=" + sessionToken + "' class='nav-btn'>Ajustes</a>";
    html += "<a href='/logout' class='nav-btn nav-logout'>Salir</a>";
    html += "</div>";
    return html;
}

String WebServerModule::getHeader() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Control de Acceso</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:0;padding:0;background:#f5f5f5;display:flex;min-height:100vh;}";
    html += ".navbar{flex:0 0 20%;background:#f8f9fa;padding:20px;display:flex;flex-direction:column;gap:15px;border-right:1px solid #dee2e6;}";
    html += ".nav-btn{display:block;padding:15px 25px;background:#007bff;color:white;text-decoration:none;border-radius:5px;text-align:center;font-size:16px;font-weight:bold;margin-bottom:5px;}";
    html += ".nav-btn:hover{background:#0056b3;}";
    html += ".nav-logout{background:#dc3545;}";
    html += ".nav-logout:hover{background:#c82333;}";
    html += ".content{flex:1;padding:20px;}";
    html += ".login-container,.control-container,.dashboard-container{max-width:none;margin:0;padding:30px;background:white;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
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
    html += ".btn-info{background:#17a2b8;color:white;}";
    html += ".btn-warning{background:#ffc107;color:black;}";
    html += ".status{text-align:center;margin:30px 0;}";
    html += ".status-open{color:#28a745;font-weight:bold;}";
    html += ".status-closed{color:#dc3545;font-weight:bold;}";
    html += ".ip-info{font-size:14px;color:#666;margin:5px 0 0 0;}";
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
    // Always allow admin access
    if (username == ADMIN_USERNAME && password == ADMIN_PASSWORD) {
        return true;
    }
    // Check configurable user credentials
    if (username == USERNAME && password == PASSWORD) {
        return true;
    }
    return false;
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

void WebServerModule::addDeviceLog(String deviceId, String action) {
    String timestamp = getCurrentTime();
    LogEntry log = {timestamp, action};
    currentDeviceLogs.push_back(log);
    if (currentDeviceLogs.size() > 100) {
        currentDeviceLogs.erase(currentDeviceLogs.begin());
    }
    // Save to device-specific file
    saveDeviceLogs(deviceId);
    // Send to WebSocket clients
    sendLogToClients(log);
}

String WebServerModule::getDeviceLogFilename(String deviceId) {
    return "/logs_" + deviceId + ".json";
}

void WebServerModule::loadDeviceLogs(String deviceId) {
    String filename = getDeviceLogFilename(deviceId);
    if (!LittleFS.exists(filename)) {
        Serial.printf("No logs file found for device %s, starting fresh\n", deviceId.c_str());
        currentDeviceLogs.clear();
        return;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.printf("Failed to open logs file for device %s\n", deviceId.c_str());
        return;
    }

    // Check file size
    size_t fileSize = file.size();
    if (fileSize > 10000) { // 10KB limit
        Serial.printf("Logs file for device %s too large, starting fresh\n", deviceId.c_str());
        file.close();
        currentDeviceLogs.clear();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("Failed to parse logs file for device %s: %s\n", deviceId.c_str(), error.c_str());
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    currentDeviceLogs.clear();

    for (JsonObject logObj : array) {
        String timestamp = logObj["timestamp"];
        String action = logObj["action"];
        currentDeviceLogs.push_back({timestamp, action});
    }

    Serial.printf("Successfully loaded %d logs for device %s\n", currentDeviceLogs.size(), deviceId.c_str());
}

void WebServerModule::saveDeviceLogs(String deviceId) {
    String filename = getDeviceLogFilename(deviceId);
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& log : currentDeviceLogs) {
        JsonObject logObj = array.add<JsonObject>();
        logObj["timestamp"] = log.timestamp;
        logObj["action"] = log.action;
        logObj["deviceId"] = deviceId;
        // Find device name and location
        for (const auto& device : devices) {
            if (device.id == deviceId) {
                logObj["deviceName"] = device.name;
                logObj["location"] = device.location;
                break;
            }
        }
    }

    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.printf("Failed to open logs file for device %s\n", deviceId.c_str());
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.printf("Failed to write logs for device %s\n", deviceId.c_str());
    }

    file.close();
}

void WebServerModule::migrateLegacyLogs() {
    // Check if legacy logs file exists
    if (!LittleFS.exists(LOGS_FILE)) {
        return; // No legacy logs to migrate
    }

    // Load legacy logs
    loadLogsFromFile();

    if (logs.empty()) {
        return; // No logs to migrate
    }

    Serial.println("Migrating legacy logs to device-specific files...");

    // Move all legacy logs to "real" device
    for (const auto& log : logs) {
        currentDeviceLogs.push_back(log);
    }

    // Save to device-specific file
    saveDeviceLogs("real");

    // Remove legacy file
    LittleFS.remove(LOGS_FILE);

    Serial.printf("Migrated %d legacy logs to device 'real'\n", logs.size());

    // Clear legacy logs vector
    logs.clear();
}

void WebServerModule::sendLogToClients(LogEntry log) {
    String message = "log:" + log.timestamp + "," + log.action;
    ws->textAll(message);
}

void WebServerModule::loadLogsFromFile() {
    if (!LittleFS.exists(LOGS_FILE)) {
        Serial.println("No logs file found, starting fresh");
        return;
    }

    File file = LittleFS.open(LOGS_FILE, "r");
    if (!file) {
        Serial.println("Failed to open logs file for reading");
        return;
    }

    // Check file size
    size_t fileSize = file.size();
    if (fileSize > 10000) { // 10KB limit
        Serial.println("Logs file too large, starting fresh");
        file.close();
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("Failed to parse logs file: %s\n", error.c_str());
        return;
    }

    JsonArray array = doc.as<JsonArray>();
    logs.clear();

    for (JsonObject logObj : array) {
        String timestamp = logObj["timestamp"];
        String action = logObj["action"];
        logs.push_back({timestamp, action});
    }

    Serial.printf("Successfully loaded %d logs from file\n", logs.size());
}

void WebServerModule::saveLogsToFile() {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();

    for (const auto& log : logs) {
        JsonObject logObj = array.add<JsonObject>();
        logObj["timestamp"] = log.timestamp;
        logObj["action"] = log.action;
    }

    File file = LittleFS.open(LOGS_FILE, "w");
    if (!file) {
        Serial.println("Failed to open logs file for writing");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write logs to file");
    }

    file.close();
}

void WebServerModule::loadUserConfig() {
    if (!LittleFS.exists("/user_config.json")) return;

    File file = LittleFS.open("/user_config.json", "r");
    if (!file) {
        Serial.println("Failed to open user config file for reading");
        return;
    }

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("Failed to parse user config file: %s\n", error.c_str());
        return;
    }

    USERNAME = doc["username"] | "admin";
    PASSWORD = doc["password"] | "admin";

    Serial.printf("Loaded user config: %s\n", USERNAME.c_str());
}

void WebServerModule::saveUserConfig() {
    DynamicJsonDocument doc(256);
    doc["username"] = USERNAME;
    doc["password"] = PASSWORD;

    File file = LittleFS.open("/user_config.json", "w");
    if (!file) {
        Serial.println("Failed to open user config file for writing");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write user config to file");
    }

    file.close();
    Serial.println("User config saved");
}

void WebServerModule::loadNetworkConfig() {
    if (!LittleFS.exists("/network_config.json")) return;

    File file = LittleFS.open("/network_config.json", "r");
    if (!file) {
        Serial.println("Failed to open network config file for reading");
        return;
    }

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("Failed to parse network config file: %s\n", error.c_str());
        return;
    }

    // Load network settings (to be used by NetworkController)
    // This is a placeholder for now
    Serial.println("Network config loaded");
}

void WebServerModule::saveNetworkConfig(bool dhcp, String ip, String gateway, String subnet, String dns1) {
    DynamicJsonDocument doc(512);
    // Save the actual network settings from the form
    doc["dhcp"] = dhcp;
    doc["ip"] = ip;
    doc["gateway"] = gateway;
    doc["subnet"] = subnet;
    doc["dns1"] = dns1;

    Serial.println("Saving network config:");
    if (dhcp) {
        Serial.println("  DHCP: enabled");
    } else {
        Serial.printf("  Static IP: %s\n", ip.c_str());
        Serial.printf("  Gateway: %s\n", gateway.c_str());
        Serial.printf("  Subnet: %s\n", subnet.c_str());
        Serial.printf("  DNS: %s\n", dns1.c_str());
    }

    File file = LittleFS.open("/network_config.json", "w");
    if (!file) {
        Serial.println("Failed to open network config file for writing");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write network config to file");
    }

    file.close();
    Serial.println("Network config saved successfully");
}

void WebServerModule::sendButtonEvent() {
    ws->textAll("button_pressed");
}

void WebServerModule::setRelayState(bool state, String deviceId) {
    bool isRealDevice = (deviceId == "real");

    if (isRealDevice) {
        relayState = state;
        digitalWrite(relayPin, state ? HIGH : LOW);
        digitalWrite(ledPin, state ? HIGH : LOW);

        // Buzzer signals for door events
        if (state) {
            buzzer->beepDoorOpen();
        } else {
            buzzer->beepDoorClose();
        }
    }

    // Log the action with device info
    String action = state ? "ABRIR" : "CERRAR";
    if (!isRealDevice) {
        action += " (DEMO - " + deviceId + ")";
    }
    addDeviceLog(deviceId, action);
}

bool WebServerModule::getRelayState() {
    return relayState;
}

void WebServerModule::toggleRelayPulse() {
    // Send a 200ms pulse to toggle the electronic lock
    digitalWrite(relayPin, HIGH);
    delay(200);
    digitalWrite(relayPin, LOW);

    // Buzzer feedback for toggle action
    buzzer->beepToggle();

    // Log the toggle action
    addDeviceLog("real", "APERTURA");
}