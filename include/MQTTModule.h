#ifndef MQTT_MODULE_H
#define MQTT_MODULE_H

#include <PubSubClient.h>
#include <NetworkClientSecure.h>
#include "NetworkController.h"
#include "ConfigLoader.h"

class MQTTModule {
private:
    PubSubClient* mqttClient;
    NetworkClientSecure netClient;
    NetworkController* netController;
    String broker;
    int port;
    String clientId;
    String username;
    String password;
    bool connected;

    // Configurable topics
    String statusTopic;
    String commandTopic;
    String sensorTopic;
    String heartbeatTopic;

    void callback(char* topic, byte* payload, unsigned int length);

public:
    MQTTModule(NetworkController* net);
    ~MQTTModule();

    void setBroker(const String& broker, int port = 8883);
    void setCredentials(const String& clientId, const String& username = "", const String& password = "");
    void setTopics(const String& status, const String& command, const String& sensor, const String& heartbeat);
    void setCACert(const char* caCert);
    void loadCertsFromSPIFFS();

    bool connect();
    void disconnect();
    bool isConnected();
    void update();

    bool publish(const char* topic, const char* payload);
    bool subscribe(const char* topic);

    // Convenience methods for configured topics
    bool publishStatus(const String& message);
    bool publishSensor(const String& sensorData);
    bool publishHeartbeat();
    bool subscribeToCommands();
};

#endif // MQTT_MODULE_H