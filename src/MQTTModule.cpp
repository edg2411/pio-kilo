#include "MQTTModule.h"

MQTTModule::MQTTModule(NetworkController* net) : netController(net), port(8883), connected(false) {
    mqttClient = new PubSubClient(netClient);
}

MQTTModule::~MQTTModule() {
    delete mqttClient;
}

void MQTTModule::setBroker(const String& broker, int port) {
    this->broker = broker;
    this->port = port;
    mqttClient->setServer(broker.c_str(), port);
    mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

void MQTTModule::setCredentials(const String& clientId, const String& username, const String& password) {
    this->clientId = clientId;
    this->username = username;
    this->password = password;
}

void MQTTModule::setTopics(const String& status, const String& command, const String& sensor, const String& heartbeat) {
    this->statusTopic = status;
    this->commandTopic = command;
    this->sensorTopic = sensor;
    this->heartbeatTopic = heartbeat;

    // Ensure callback is set up with the updated topics
    if (mqttClient) {
        mqttClient->setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->callback(topic, payload, length);
        });
    }
}

void MQTTModule::setCACert(const char* caCert) {
    netClient.setCACert(caCert);
}

void MQTTModule::loadCertsFromSPIFFS() {
    static bool initialLoad = true;

    if (initialLoad) {
        Serial.println("Loading certificates from SPIFFS...");
        initialLoad = false;
    } else {
        Serial.println("Reloading certificates for reconnection...");
    }

    // Load CA certificate
    String caCert = ConfigLoader::loadCACert();
    if (caCert.length() > 0 && caCert.length() < 10000) {  // Reasonable size check
        if (initialLoad) Serial.printf("Setting CA certificate (%d bytes)\n", caCert.length());
        netClient.setCACert(caCert.c_str());
    } else if (caCert.length() >= 10000) {
        Serial.println("CA certificate too large, skipping");
    } else {
        if (initialLoad) Serial.println("No CA certificate found");
    }

    // Load client certificate
    String clientCert = ConfigLoader::loadClientCert();
    if (clientCert.length() > 0 && clientCert.length() < 10000) {
        if (initialLoad) Serial.printf("Setting client certificate (%d bytes)\n", clientCert.length());
        netClient.setCertificate(clientCert.c_str());
    } else if (clientCert.length() >= 10000) {
        Serial.println("Client certificate too large, skipping");
    } else {
        if (initialLoad) Serial.println("No client certificate found");
    }

    // Load private key
    String privateKey = ConfigLoader::loadPrivateKey();
    if (privateKey.length() > 0 && privateKey.length() < 10000) {
        if (initialLoad) Serial.printf("Setting private key (%d bytes)\n", privateKey.length());
        netClient.setPrivateKey(privateKey.c_str());
    } else if (privateKey.length() >= 10000) {
        Serial.println("Private key too large, skipping");
    } else {
        if (initialLoad) Serial.println("No private key found");
    }

    if (initialLoad) {
        Serial.println("Certificate loading completed");
    } else {
        Serial.println("Certificate reloading completed");
    }
}

bool MQTTModule::connect() {
    if (connected || broker.isEmpty()) return false;

    Serial.println("Attempting MQTT connection...");

    // Reload certificates before connecting (in case of corruption)
    loadCertsFromSPIFFS();

    mqttClient->setServer(broker.c_str(), port);

    if (mqttClient->connect(clientId.c_str(), username.c_str(), password.c_str())) {
        connected = true;
        Serial.println("✅ MQTT connected successfully");

        // Subscribe to command topic after successful connection
        if (!commandTopic.isEmpty()) {
            delay(100);  // Small delay before subscribing
            if (subscribe(commandTopic.c_str())) {
                Serial.println("✅ Subscribed to command topic: " + commandTopic);
            } else {
                Serial.println("❌ Failed to subscribe to command topic");
            }
        }

        return true;
    }

    Serial.println("❌ MQTT connection failed");
    return false;
}

void MQTTModule::disconnect() {
    if (connected) {
        Serial.println("MQTT disconnecting...");
        mqttClient->disconnect();
        connected = false;

        // Reset NetworkClientSecure state
        netClient.stop();
        delay(100);  // Allow time for cleanup

        Serial.println("MQTT disconnected and cleaned up");
    }
}

bool MQTTModule::isConnected() {
    bool wasConnected = connected;
    connected = mqttClient->connected();

    // If we just disconnected, clean up
    if (wasConnected && !connected) {
        Serial.println("MQTT connection lost, cleaning up...");
        netClient.stop();
        delay(100);
    }

    return connected;
}

void MQTTModule::update() {
    static unsigned long lastReconnectAttempt = 0;
    const unsigned long reconnectDelay = 5000;  // 5 seconds between reconnection attempts

    if (connected) {
        mqttClient->loop();
    } else if (netController->getState() == CONNECTED) {
        // Only attempt reconnection if enough time has passed
        if (millis() - lastReconnectAttempt > reconnectDelay) {
            Serial.println("Network is connected, attempting MQTT reconnection...");
            lastReconnectAttempt = millis();
            connect();
        }
    }
}

bool MQTTModule::publish(const char* topic, const char* payload) {
    if (!connected) return false;
    return mqttClient->publish(topic, payload);
}

bool MQTTModule::subscribe(const char* topic) {
    if (!connected) return false;
    return mqttClient->subscribe(topic);
}

void MQTTModule::callback(char* topic, byte* payload, unsigned int length) {
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
bool MQTTModule::publishStatus(const String& message) {
    if (statusTopic.isEmpty()) return false;
    return publish(statusTopic.c_str(), message.c_str());
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