#include <Arduino.h>

#include <PubSubClient.h>
#include <WiFi.h>
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

#include "mqtt.h"
#include "constants.h"


MQTT::MQTT() {
    currentScore = 0;
    maxScore = 1;
}

void MQTT::incomingCallback(char* topic, uint8_t* payload, unsigned int length) {
  Serial.println("Received on: " + String(topic));
  uint16_t incoming = atoi((char*)payload);
  // Atoi returns 0 on error, so no problem
  if (incoming > maxScore) {
    maxScore = incoming;
    Serial.printf("Increased max to %d\n", incoming);
  }
  
}

void MQTT::connect() {
  mqttClient.setServer(mqttServer, mqttPort);
  while (!mqttClient.connected()) {
    mqttClient.connect(CUP_ID);
    Serial.print(".");
    delay(500);
  }
  Serial.println("MQTT connected");
  mqttClient.subscribe("cup/+/imageCount");
  mqttClient.setCallback(std::bind(&MQTT::incomingCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}


void MQTT::publishScore() {
  currentScore++;
  // Reduce unnecessary traffic
  if(currentScore < maxScore) return;

  char topic[32];
  sprintf(topic, "cup/%s/imageCount", CUP_ID);
  char value[7];
  sprintf(value, "%d", currentScore);

  Serial.printf("Publishing new score %s\n", value);

  mqttClient.publish(topic, value);
}

void MQTT::handle() {
  mqttClient.loop();
  if (!mqttClient.connected()) {
    Serial.println("Lost MQTT connection... trying reconnect");
    connect();
  }
}
