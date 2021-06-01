/**
 *  @filename   :   main.cpp
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

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "sys/time.h"
#include "weatherbase.h"
#include "wifiwithmqtt.h"
#include "espnow.h"
#include "display.h"
#include "HTU21D.h"

extern bool buttonLongPress;
uint16_t count=0;
bool dataValid=false;
sensor_data_t sensorData;
float currentSecs;
float prevSecs=0;
long last_reconnect=30000;
bool htd21Init = false;

HTU21D            myHTU21D(HTU21D_RES_RH12_TEMP14);

void connectEspNow(void);
void connectWiFi(void);

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  
  struct timeval tv;
  gettimeofday(&tv,NULL);
  currentSecs=tv.tv_sec + (tv.tv_usec / 1E06);

  Serial.printf("Current Time %f\n", currentSecs);

  for(int i=0;i<6;i++) {
    Serial.printf("%x:",mac_addr[i]);

  }
  Serial.println();

  if (len == sizeof(sensor_data_t))
  {
    memcpy(&sensorData, data, len);

    Serial.printf("Wakeup Reason=%d\n", sensorData.wakeup_reason);
    Serial.printf("Temperature=%f *C\n",sensorData.temperature);
    Serial.printf("Pressure=%d Pa\n",sensorData.pressure);
    Serial.printf("Humidity=%f\n",sensorData.humidity);
    Serial.printf("Battery Volts=%f mV\n",sensorData.battery_millivolts);
    Serial.printf("Direction=%d\n",sensorData.direction);
    Serial.printf("Rain Count=%f\n", sensorData.rain);
    Serial.printf("Anenomoeter Count=%f\n", sensorData.wind_speed);
    Serial.printf("Count=%d\n",count++);

    dataValid=true;


  } else {
    Serial.println("Received something of wrong length");
  }
}

void sendMQTTData() {
  float messageInterval;
  if(prevSecs==0) {
    messageInterval = currentSecs;
  } else {
    messageInterval = currentSecs - prevSecs;
  }

  Serial.printf("Message Interval %f\n",messageInterval);

  publishData(sensorData.wakeup_reason, sensorData.temperature, sensorData.pressure, sensorData.humidity, sensorData.battery_millivolts, sensorData.direction, sensorData.wind_speed, sensorData.rain);
  prevSecs = currentSecs;

  if(!htd21Init){
    Wire.begin();
    Wire.setClock(100000);
    if(myHTU21D.begin() !=true) {
      Serial.println("HTU21 Failed");
      log("htd21","HTD21D Failed to Initialize");
      setError("HTD21D Initialization Failure");
    } else {
      htd21Init = true;
    }
  }

  float roomC = 0.0;
  float roomHum = 0.0;
  if(htd21Init) {
    roomC = myHTU21D.readTemperature();
    roomHum = myHTU21D.readCompensatedHumidity();
    if(roomC > 65.0) {
      log("htd21d","HTD21D Failure");
      htd21Init = false;
      setError("Temperature Sensor Failure");
    }
    publishRoomStats(roomC,roomHum);
  }

  displayData(sensorData.temperature, sensorData.pressure, sensorData.humidity, sensorData.battery_millivolts, sensorData.direction, sensorData.wind_speed, sensorData.rain, roomC, roomHum);

}

void connectEspNow() {
  esp_now_init();

  esp_now_register_recv_cb(espnow_recv_cb);

}

void connectWiFi(){

  // The mode must be AP_STA even if you do not activate the softAP, or it doesn't work
  // Configuring the softAP is not required, though, but it is a way to get the MAC Address and channel
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.begin();
  if(WiFi.softAP(STATION_NAME,"1234567890",1,0))
  {
    Serial.println("Soft AP Success)");
  } else {
    Serial.println("SoftAP Fail");
  }

  uint8_t wifiConn=0;
  while (WiFi.status() != WL_CONNECTED) { 
    wifiConn++;
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
    Serial.println(WiFi.status());
    if(wifiConn > 10)
      ESP.restart();
  }

  otaSetup();
  initMQTT();
}

void setup() {
  Serial.begin(115200);
  readEEPROM();
  connectWiFi();
  connectEspNow();
  WiFi.printDiag(Serial);
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  pinMode(CONFIG_BUTTON,INPUT);
  attachInterrupt(digitalPinToInterrupt(CONFIG_BUTTON), longPress, CHANGE);

  log("main","Starting");
  
  initDisplay();
}

void loop() {

  if((WiFi.status() != WL_CONNECTED) && (millis() > last_reconnect)) {
    Serial.println("Wifi Reconnect");
    WiFi.disconnect();
    WiFi.begin();
    last_reconnect = millis() + 30000;
    Serial.printf("WiFi Status=%d\n", WiFi.status());
  }

  if(buttonLongPress) {
    Serial.println("Config Button");
    callWFM(false);
    buttonLongPress = false;
  }

  // Fails if I send data to MQTT in call back
  if(dataValid) {
    dataValid=false;
    Serial.println("Sending Data");
    sendMQTTData();
    Serial.printf("WifiStatus %d\n",WiFi.status());
  }

  mqttLoop();
  displayLoop();

  ArduinoOTA.handle();
}