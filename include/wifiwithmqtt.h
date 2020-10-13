/**
 *  @filename   :   wifiwithmqtt.h
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

#ifndef INCLUDE_WIFIWITHMQTT_H_
#define INCLUDE_WIFIWITHMQTT_H_

#define MQTT_SERVER_LENGTH 30
#define MQTT_TOPIC_LENGTH 40


#define CONFIG_BUTTON 25

struct mqttConfig {
  uint32_t valid;
  char server[MQTT_SERVER_LENGTH];
  uint16_t port;
  char topic[MQTT_TOPIC_LENGTH]; 
};

ICACHE_RAM_ATTR void longPress(void);
void callWFM(bool);
void readEEPROM(void);
void initializeWifiWithMQTT(void);
boolean publishData(uint8_t reason, float temperature, int32_t pressure, float humidity, float battery_millivolts, uint16_t direction, float anemometer, float rain);
void initMQTT();
void disconnectMQTT();
#endif /* INCLUDE_WIFIWITHMQTT_H_ */
