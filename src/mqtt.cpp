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

String mqttStateText(int state) {
    switch (state) {
        case MQTT_CONNECTION_TIMEOUT:      return "connection timeout";
        case MQTT_CONNECTION_LOST:         return "connection lost";
        case MQTT_CONNECT_FAILED:          return "connect failed";
        case MQTT_DISCONNECTED:            return "disconnected";
        case MQTT_CONNECT_BAD_PROTOCOL:    return "bad protocol";
        case MQTT_CONNECT_BAD_CLIENT_ID:   return "bad client id";
        case MQTT_CONNECT_UNAVAILABLE:     return "unavailable";
        case MQTT_CONNECT_BAD_CREDENTIALS: return "bad credentials";
        case MQTT_CONNECT_UNAUTHORIZED:    return "unauthorized";
        default:                           return String(state);
    }
}

void checkMqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    if (!mqttClient.connected()) {
        Log("MQTT: " + mqttStateText(mqttClient.state()));
        Log("Attempting MQTT connection... ");
        // Create a random client ID
        String clientId = "optolink";
        //clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PW)) {
            Log("connected");
        
        } else {
            Log("MQTT: " + mqttStateText(mqttClient.state()));
        }
    }
}