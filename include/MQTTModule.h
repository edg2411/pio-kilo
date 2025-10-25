#ifndef MQTT_MODULE_H
#define MQTT_MODULE_H

#include "mqtt_client.h"
#include "NetworkController.h"
#include "ConfigLoader.h"

class MQTTModule {
private:
    esp_mqtt_client_handle_t mqttClient;
    esp_mqtt_client_config_t mqttConfig;
    NetworkController* netController;
    String broker;
    int port;
    String brokerUri;
    String clientId;
    String username;
    String password;
    String caCertPem;
    String clientCertPem;
    String privateKeyPem;
    bool connected;
    int defaultQoS;
    bool useMQTT5;

    // Configurable topics
    String statusTopic;
    String commandTopic;
    String sensorTopic;
    String heartbeatTopic;

    void rebuildClient();
    bool isConfigReady() const;

public:
    // Helper methods for event handler
    void setConnected(bool state) { connected = state; }
    const String& getCommandTopic() const { return commandTopic; }
    void handleMessage(char* topic, byte* payload, unsigned int length);

private:

public:
    MQTTModule(NetworkController* net);
    ~MQTTModule();

    void setBroker(const String& broker, int port = 8883);
    void setCredentials(const String& clientId, const String& username = "", const String& password = "");
    void setTopics(const String& status, const String& command, const String& sensor, const String& heartbeat);
    void setCACert(const char* caCert);
    void loadCertsFromSPIFFS();

    // QoS and MQTT5 configuration
    void setQoS(int qos = 0);  // 0, 1, or 2
    void setMQTT5(bool enable = false);

    bool connect();
    void disconnect();
    bool isConnected();
    void update();

    bool publish(const char* topic, const char* payload, int qos = -1, bool retain = false);  // -1 uses default QoS
    bool subscribe(const char* topic, int qos = -1);  // -1 uses default QoS

    // Convenience methods for configured topics
    bool publishStatus(const String& message, bool retain = false);
    bool publishSensor(const String& sensorData);
    bool publishHeartbeat();
    bool subscribeToCommands();
};

#endif // MQTT_MODULE_H