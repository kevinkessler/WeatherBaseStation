/**
 *  @filename   :   espnow.cpp
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
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <sys/time.h>
#include "weatherbase.h"
#include "espnow.h"

static uint8_t broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint64_t lastTime=0;

static void handle_error(esp_err_t err)
{
  switch (err)
  {
    case ESP_ERR_ESPNOW_NOT_INIT:
      Serial.println("Not init");
      break;

    case ESP_ERR_ESPNOW_ARG:
      Serial.println("Argument invalid");
      break;

    case ESP_ERR_ESPNOW_INTERNAL:
      Serial.println("Internal error");
      break;

    case ESP_ERR_ESPNOW_NO_MEM:
      Serial.println("Out of memory");
      break;

    case ESP_ERR_ESPNOW_NOT_FOUND:
      Serial.println("Peer is not found");
      break;

    case ESP_ERR_ESPNOW_IF:
      Serial.println("Current WiFi interface doesn't match that of peer");
      break;

    default:
      break;
  }
}

/*static void msg_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  struct timeval tv;
  gettimeofday(&tv,NULL);

  uint64_t curTime=tv.tv_sec * 1E06 + tv.tv_usec;

  if (len == sizeof(esp_now_msg_t))
  {
    esp_now_msg_t msg;
    memcpy(&msg, data, len);

    Serial.print("Counter: ");
    Serial.println(msg.counter);
    Serial.printf("uSecs: %lld\n",curTime-lastTime);
  }
  lastTime=curTime;
}

static void msg_send_cb(const uint8_t* mac, esp_now_send_status_t sendStatus)
{

  switch (sendStatus)
  {
    case ESP_NOW_SEND_SUCCESS:
      Serial.println("Send success");
      break;

    case ESP_NOW_SEND_FAIL:
      Serial.println("Send Failure");
      break;

    default:
      break;
  }
}*/

void send_msg(uint8_t* msg, uint16_t packet_size)
{
  // Pack
  uint8_t msg_data[packet_size];
  memcpy(&msg_data[0], msg, packet_size);

  esp_err_t status = esp_now_send(broadcast_mac, msg_data, packet_size);
  if (ESP_OK != status)
  {
    Serial.println("Error sending message");
    handle_error(status);
  }
}

bool espnowInit(esp_now_recv_cb_t recv_cb, esp_now_send_cb_t send_cb)
{
  //Puts ESP in STATION MODE
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  WiFi.printDiag(Serial);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(3,WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial);

  if (esp_now_init() != 0)
  {
    return false;
  }

  /*esp_now_peer_info_t peer_info;
  peer_info.channel = WIFI_CHANNEL;
  memcpy(peer_info.peer_addr, broadcast_mac, 6);
  peer_info.ifidx = ESP_IF_WIFI_STA;
  peer_info.encrypt = false;
  esp_err_t status = esp_now_add_peer(&peer_info);
  if (ESP_OK != status)
  {
    Serial.println("Could not add peer");
    handle_error(status);
    return false;
  }*/

  // Set up callback
  if(recv_cb != NULL) {
    esp_err_t status = esp_now_register_recv_cb(recv_cb);
    if (ESP_OK != status)
    {
        Serial.println("Could not register callback");
        handle_error(status);
        return false;
    }
  }

  if(send_cb != NULL) {
    esp_err_t status = esp_now_register_send_cb(send_cb);
    if (ESP_OK != status)
    {
        Serial.println("Could not register send callback");
        handle_error(status);
        return false;
    }
  }

  return true;
}
