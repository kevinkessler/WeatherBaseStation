/**
 *  @filename   :   wifi.cpp
 *  @brief      :   ESP32 Weather Base Station
 *
 *  @author     :   Kevin Kessler
 *
 * Copyright (C) 2020 Kevin Kessler
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <WiFi.h> 
#include "PubSubClient.h"
#include "weatherbase.h"
#include "wifiwithmqtt.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
char subName[25];

extern char mqttServer[MQTT_SERVER_LENGTH];
extern char mqttTopic[MQTT_TOPIC_LENGTH];
extern uint16_t mqttPort;

const char *stateJson = "{\"wakeup_reason\":%d,\"temperature\":%.1f,\"pressure\":%d,\"humidity\":%.1f,\"battery\":%.1f,\"direction\":%d,\"anemometer\":%f,\"rain\":%f}";

void mqttCallback(char *topic, byte *payload, uint16_t length) {
    Serial.printf("Message Received on topic %s\n", topic);
}

static void reconnect() {
    if(mqttClient.connect(subName)) {
        Serial.println("MQTT Connected");
    } else {
        char errorMes[50];
        sprintf(errorMes, "MQTT Connection failed, rc=%d",mqttClient.state());
        Serial.println(errorMes);
    }

}

static boolean publishMes(char *topic, char *payload) {
    if(!mqttClient.connected())
        reconnect();
        if(!mqttClient.connected())
            return false;

    if(!mqttClient.publish(topic, payload)) {
        char errorMes[50];
        sprintf(errorMes, "MQTT Publish failed, rc=%d",mqttClient.state());
        Serial.println(errorMes);
        return false;
    }

    return true;
}

boolean publishData(uint8_t reason, float temperature, int32_t pressure, float humidity, float battery_millivolts, uint16_t direction, float anemometer, float rain) {

    char payload[200];
    sprintf(payload,stateJson,reason,temperature,pressure,humidity,battery_millivolts,direction,anemometer,rain);
    Serial.println(payload);

//    char topic[MQTT_TOPIC_LENGTH + 10];
//    sprintf(topic,"%s/%s",mqttTopic,"state");

    return publishMes(mqttTopic,payload);
}

void disconnectMQTT() {
    mqttClient.disconnect();
}

void initMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    
    sprintf(subName, "weathertest-%s", &(WiFi.macAddress().c_str())[9]);
    Serial.println(subName);

}