#include "mqtt.h"
#include "secrets.h"

WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);


void setupMqtt() {
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
}

void publishMqtt(String topic, const char* payload) {
    Log("publish " + topic_prefix + topic + "->" + String(payload));
    mqttClient.publish((topic_prefix + topic).c_str(), payload);
}

void checkMqtt() {
    if (!mqttClient.connected()) {
        Log("Attempting MQTT connection... ");
        // Create a random client ID
        String clientId = "optolink";
        //clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PW)) {
            Log("connected");
        
        } else {
            Log(String(mqttClient.state()));
        }
    }
}