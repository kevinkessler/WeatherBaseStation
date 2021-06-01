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

#ifdef DEV_MODE
#include "display.h"
#include <ArduinoJson.h>
#include "HTU21D.h"

static boolean publishMes(char *topic, char *payload);

HTU21D            myHTU21D(HTU21D_RES_RH12_TEMP14);

#endif

WiFiClient espClient;
PubSubClient mqttClient(espClient);
char subName[25];

extern char mqttServer[MQTT_SERVER_LENGTH];
extern char mqttTopic[MQTT_TOPIC_LENGTH];
extern uint16_t mqttPort;

const char *stateJson = "{\"wakeup_reason\":%d,\"temperature\":%.1f,\"pressure\":%d,\"humidity\":%.1f,\"battery\":%.1f,\"direction\":%d,\"anemometer\":%f,\"rain\":%f}";
const char *roomData="{\"room_temp\":%.1f,\"room_hum\":%.1f}";
const char *logJson="{\"host\":\"%.32s\",\"system\":\"%.20s\",\"message\":\"%.150s\"}";

void mqttCallback(char *topic, byte *payload, uint16_t length) {
    Serial.printf("Message Received on topic %s\n", topic);
    if(length < 50) //stop it from acting own it's own messages
        return;

    #ifdef DEV_MODE
    Serial.print("Doc Length ");
    Serial.println(length);

    StaticJsonDocument<256> doc;
    deserializeJson(doc, (const byte *)payload, length);

    Serial.print("Temperature ");
    float temperature=doc["temperature"];
    Serial.println(temperature);

    int32_t pressure=doc["pressure"];
    Serial.print("Pressure ");
    Serial.println(pressure);

    float humidity=doc["humidity"];
    Serial.print("Humidity ");
    Serial.println(humidity);

    float batt=doc["battery"];
    Serial.print("Battery ");
    Serial.println(batt);

    uint16_t dir = doc["direction"];
    Serial.print("Direction ");
    Serial.println(dir);

    float anem = doc["anemometer"];
    Serial.print("Wind ");
    Serial.println(anem);

    float rain = doc["rain"];
    Serial.print("Rain ");
    Serial.println(rain);


    //Sanity Check this data
    float roomC = myHTU21D.readTemperature();
    float roomHum = myHTU21D.readCompensatedHumidity();

    if((roomC>0.0)&&(roomC<65.0)) {
        char jdata[200];
        sprintf(jdata,roomData,roomC,roomHum);
        Serial.println(jdata);
        publishMes(mqttTopic,jdata);
    }

    displayData(temperature, pressure, humidity, batt, dir, anem, rain, roomC, roomHum);
    #endif
    
}

static void reconnect() {
    if(mqttClient.connect(subName)) {
        Serial.println("MQTT Connected");
    } else {
        char errorMes[50];
        sprintf(errorMes, "MQTT Connection failed, rc=%d",mqttClient.state());
        Serial.println(errorMes);
    }
#ifdef DEV_MODE
    Serial.print("Subscribing to ");
    Serial.print(mqttTopic);
    mqttClient.subscribe(mqttTopic);
#endif
}

static boolean publishMes(const char *topic, const char *payload) {
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


    return publishMes(mqttTopic,payload);
}

void disconnectMQTT() {
    mqttClient.disconnect();
}

void publishRoomStats(float roomC, float roomHum) {

    if((roomC>0.0)&&(roomC<65.0)) {
      char jdata[200];
      sprintf(jdata,roomData,roomC,roomHum);
      Serial.println(jdata);
      publishMes(mqttTopic,jdata);
    }
}

void logMessage(const char*system, const char*message) {
    char payload[250];

    sprintf(payload,logJson,STATION_NAME, system, message);
    publishMes(LOG_TOPIC,payload);
}

void initMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);

    sprintf(subName, "weathertest-%s", &(WiFi.macAddress().c_str())[9]);
    Serial.println(subName);

    #ifdef DEV_MODE
    reconnect();

    if(myHTU21D.begin() !=true)
        Serial.println("HTU21 Failed");
    #endif

}

void mqttLoop(void) {
    if(!mqttClient.connected())
        reconnect();
    
    mqttClient.loop();
}
