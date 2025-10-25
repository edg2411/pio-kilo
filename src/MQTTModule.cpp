#include "MQTTModule.h"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

MQTTModule::MQTTModule(NetworkController* net)
    : mqttClient(nullptr), netController(net), port(8883), connected(false), defaultQoS(0), useMQTT5(false) {
    mqttConfig = {};
    mqttConfig.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
}

MQTTModule::~MQTTModule() {
    if (mqttClient) {
        esp_mqtt_client_stop(mqttClient);
        esp_mqtt_client_destroy(mqttClient);
        mqttClient = nullptr;
    }
}

void MQTTModule::setBroker(const String& broker, int port) {
    this->broker = broker;
    this->port = port;

    brokerUri = (port == 8883 ? "mqtts://" : "mqtt://");
    brokerUri += broker;
    brokerUri += ":";
    brokerUri += String(port);

    mqttConfig.broker.address.uri = brokerUri.c_str();
    mqttConfig.broker.address.port = port;
#ifdef MQTT_TRANSPORT_OVER_SSL
    mqttConfig.broker.address.transport = (port == 8883) ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP;
#endif

    rebuildClient();
}

void MQTTModule::setCredentials(const String& clientId, const String& username, const String& password) {
    this->clientId = clientId;
    this->username = username;
    this->password = password;

    mqttConfig.credentials.client_id = this->clientId.isEmpty() ? nullptr : this->clientId.c_str();
    mqttConfig.credentials.username = this->username.isEmpty() ? nullptr : this->username.c_str();
    mqttConfig.credentials.authentication.password = this->password.isEmpty() ? nullptr : this->password.c_str();

    rebuildClient();
}

void MQTTModule::setTopics(const String& status, const String& command, const String& sensor, const String& heartbeat) {
    this->statusTopic = status;
    this->commandTopic = command;
    this->sensorTopic = sensor;
    this->heartbeatTopic = heartbeat;
}

void MQTTModule::setCACert(const char* caCert) {
    if (caCert) {
        caCertPem = String(caCert);
    } else {
        caCertPem = "";
    }

    mqttConfig.broker.verification.certificate = caCertPem.isEmpty() ? nullptr : caCertPem.c_str();
    mqttConfig.broker.verification.certificate_len = caCertPem.isEmpty() ? 0 : caCertPem.length() + 1;

    rebuildClient();
}

void MQTTModule::setQoS(int qos) {
    if (qos >= 0 && qos <= 2) {
        defaultQoS = qos;
    }
}

void MQTTModule::setMQTT5(bool enable) {
    bool supported = false;
#if defined(CONFIG_MQTT_PROTOCOL_5)
    supported = CONFIG_MQTT_PROTOCOL_5;
#endif

    if (enable && !supported) {
        Serial.println("MQTT v5 support is not enabled in this build; falling back to MQTT 3.1.1");
        useMQTT5 = false;
    } else {
        useMQTT5 = enable;
    }

    mqttConfig.session.protocol_ver = useMQTT5 ? MQTT_PROTOCOL_V_5 : MQTT_PROTOCOL_V_3_1_1;
    rebuildClient();
}

void MQTTModule::loadCertsFromSPIFFS() {
    static bool initialLog = true;
    bool firstRun = initialLog;

    if (firstRun) {
        Serial.println("Loading certificates from SPIFFS...");
        initialLog = false;
    } else {
        Serial.println("Reloading certificates for reconnection...");
    }

    bool updated = false;

    // Load CA certificate
    String caCert = ConfigLoader::loadCACert();
    if (caCert.length() > 0 && caCert.length() < 10000) {  // Reasonable size check
        if (firstRun) {
            Serial.printf("Setting CA certificate (%d bytes)\n", caCert.length());
        }
        if (caCert != caCertPem) {
            caCertPem = caCert;
            mqttConfig.broker.verification.certificate = caCertPem.c_str();
            mqttConfig.broker.verification.certificate_len = caCertPem.length() + 1;
            updated = true;
        }
    } else if (caCert.length() >= 10000) {
        Serial.println("CA certificate too large, skipping");
    } else {
        if (firstRun) {
            Serial.println("No CA certificate found");
        }
        if (!caCertPem.isEmpty()) {
            caCertPem = "";
            mqttConfig.broker.verification.certificate = nullptr;
            mqttConfig.broker.verification.certificate_len = 0;
            updated = true;
        }
    }

    // Load client certificate (optional)
    String clientCert = ConfigLoader::loadClientCert();
    if (clientCert.length() > 0 && clientCert.length() < 10000) {
        if (firstRun) {
            Serial.printf("Setting client certificate (%d bytes)\n", clientCert.length());
        }
        if (clientCert != clientCertPem) {
            clientCertPem = clientCert;
            mqttConfig.credentials.authentication.certificate = clientCertPem.c_str();
            updated = true;
        }
    } else if (clientCert.length() >= 10000) {
        Serial.println("Client certificate too large, skipping");
    } else if (!clientCertPem.isEmpty()) {
        clientCertPem = "";
        mqttConfig.credentials.authentication.certificate = nullptr;
        updated = true;
    }

    // Load private key (optional)
    String privateKey = ConfigLoader::loadPrivateKey();
    if (privateKey.length() > 0 && privateKey.length() < 10000) {
        if (firstRun) {
            Serial.printf("Setting private key (%d bytes)\n", privateKey.length());
        }
        if (privateKey != privateKeyPem) {
            privateKeyPem = privateKey;
            mqttConfig.credentials.authentication.key = privateKeyPem.c_str();
            updated = true;
        }
    } else if (privateKey.length() >= 10000) {
        Serial.println("Private key too large, skipping");
    } else if (!privateKeyPem.isEmpty()) {
        privateKeyPem = "";
        mqttConfig.credentials.authentication.key = nullptr;
        updated = true;
    }

    if (updated) {
        rebuildClient();
    }

    if (firstRun) {
        Serial.println("Certificate loading completed");
    } else {
        Serial.println("Certificate reloading completed");
    }
}

bool MQTTModule::connect() {
    if (connected) {
        return true;
    }

    if (!isConfigReady()) {
        Serial.println("MQTT broker not configured, cannot connect");
        return false;
    }

    Serial.println("Attempting MQTT connection...");

    // Reload certificates before connecting (in case of corruption)
    loadCertsFromSPIFFS();

    if (!mqttClient) {
        rebuildClient();
    }

    if (!mqttClient) {
        Serial.println("❌ Failed to initialize MQTT client handle");
        return false;
    }

    esp_err_t err = esp_mqtt_client_start(mqttClient);
    if (err == ESP_OK) {
        Serial.println("✅ MQTT client started successfully");
        // Connection status will be updated via event handler
        return true;
    } else {
        Serial.printf("❌ Failed to start MQTT client: %s\n", esp_err_to_name(err));
        return false;
    }
}

void MQTTModule::disconnect() {
    if (!mqttClient) {
        connected = false;
        return;
    }

    if (connected) {
        Serial.println("MQTT disconnecting...");
        esp_mqtt_client_stop(mqttClient);
        connected = false;
        Serial.println("MQTT disconnected");
    } else {
        esp_mqtt_client_stop(mqttClient);
    }
}

bool MQTTModule::isConnected() {
    return connected;
}

void MQTTModule::update() {
    static unsigned long lastReconnectAttempt = 0;
    const unsigned long reconnectDelay = 5000;  // 5 seconds between reconnection attempts

    if (!connected && netController->getState() == CONNECTED) {
        // Only attempt reconnection if enough time has passed
        if (millis() - lastReconnectAttempt > reconnectDelay) {
            Serial.println("Network is connected, attempting MQTT reconnection...");
            lastReconnectAttempt = millis();
            connect();
        }
    }
}

bool MQTTModule::publish(const char* topic, const char* payload, int qos, bool retain) {
    if (!connected || !mqttClient) return false;

    int actualQoS = (qos == -1) ? defaultQoS : qos;
    int msg_id = esp_mqtt_client_publish(mqttClient, topic, payload, strlen(payload), actualQoS, retain ? 1 : 0);
    return (msg_id > 0);
}

bool MQTTModule::subscribe(const char* topic, int qos) {
    if (!connected || !mqttClient) return false;

    int actualQoS = (qos == -1) ? defaultQoS : qos;
    int msg_id = esp_mqtt_client_subscribe(mqttClient, topic, actualQoS);
    return (msg_id > 0);
}

void MQTTModule::handleMessage(char* topic, byte* payload, unsigned int length) {
    // Handle incoming messages
    Serial.print("MQTT Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Debug: Print configured command topic
    Serial.print("Configured command topic: ");
    Serial.println(commandTopic);

    // Handle commands
    String topicStr = String(topic);
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Comparing '");
    Serial.print(topicStr);
    Serial.print("' with '");
    Serial.print(commandTopic);
    Serial.println("'");

    if (topicStr == commandTopic) {
        Serial.print("✅ Received command: ");
        Serial.println(message);
        // Handle command here
    } else {
        Serial.println("❌ Topic does not match command topic");
    }
}

// Convenience methods for configured topics
bool MQTTModule::publishStatus(const String& message, bool retain) {
    if (statusTopic.isEmpty()) return false;
    return publish(statusTopic.c_str(), message.c_str(), -1, retain);
}

bool MQTTModule::publishSensor(const String& sensorData) {
    if (sensorTopic.isEmpty()) return false;
    return publish(sensorTopic.c_str(), sensorData.c_str());
}

bool MQTTModule::publishHeartbeat() {
    if (heartbeatTopic.isEmpty()) return false;
    String heartbeatMsg = "{\"timestamp\":" + String(millis()) + ",\"status\":\"online\"}";
    return publish(heartbeatTopic.c_str(), heartbeatMsg.c_str());
}

bool MQTTModule::subscribeToCommands() {
    if (commandTopic.isEmpty()) return false;
    return subscribe(commandTopic.c_str());
}

void MQTTModule::rebuildClient() {
    if (!isConfigReady()) {
        return;
    }

    if (mqttClient) {
        esp_mqtt_client_stop(mqttClient);
        esp_mqtt_client_destroy(mqttClient);
        mqttClient = nullptr;
        connected = false;
    }

    mqttConfig.session.protocol_ver = useMQTT5 ? MQTT_PROTOCOL_V_5 : MQTT_PROTOCOL_V_3_1_1;

    mqttClient = esp_mqtt_client_init(&mqttConfig);
    if (!mqttClient) {
        Serial.println("❌ Unable to initialize MQTT client with current configuration");
        return;
    }

    esp_err_t err = esp_mqtt_client_register_event(mqttClient, MQTT_EVENT_ANY, mqtt_event_handler, this);
    if (err != ESP_OK) {
        Serial.printf("❌ Failed to register MQTT event handler: %s\n", esp_err_to_name(err));
    }
}

bool MQTTModule::isConfigReady() const {
    return !brokerUri.isEmpty();
}

// MQTT Event Handler
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    MQTTModule* mqttModule = (MQTTModule*)handler_args;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            mqttModule->setConnected(true);
            Serial.println("✅ MQTT connected successfully");

            // Subscribe to command topic after successful connection
            if (!mqttModule->getCommandTopic().isEmpty()) {
                delay(100);  // Small delay before subscribing
                if (mqttModule->subscribe(mqttModule->getCommandTopic().c_str())) {
                    Serial.println("✅ Subscribed to command topic: " + mqttModule->getCommandTopic());
                } else {
                    Serial.println("❌ Failed to subscribe to command topic");
                }
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqttModule->setConnected(false);
            Serial.println("❌ MQTT disconnected");
            break;

        case MQTT_EVENT_DATA:
            // Handle incoming message
            if (event->topic && event->data) {
                char topic[event->topic_len + 1];
                memcpy(topic, event->topic, event->topic_len);
                topic[event->topic_len] = '\0';

                mqttModule->handleMessage(topic, (byte*)event->data, event->data_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            Serial.println("MQTT_EVENT_ERROR");
            break;

        default:
            break;
    }
}