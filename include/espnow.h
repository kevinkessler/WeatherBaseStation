/**
 *  @filename   :   espnow.h
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

#ifndef INCLUDE_ESPNOW_H_
#define INCLUDE_ESPNOW_H_

#define WIFI_CHANNEL (3)

bool espnowInit(esp_now_recv_cb_t recv_cb, esp_now_send_cb_t send_cb);
void send_msg(uint8_t* msg, uint16_t packet_size);

#endif /* INCLUDE_ESPNOW_H_ */