/**
 *  @filename   :   weatherbase.h
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

#ifndef INCLUDE_WEATHERBASE_H_
#define INCLUDE_WEATHERBASE_H_

#define STATION_NAME "WeatherBase"

typedef struct __attribute__((packed)) sensor_data_t {
    uint8_t wakeup_reason;
    float temperature;
    int32_t pressure;
    float humidity;
    float battery_millivolts;
    uint16_t direction;
    uint16_t anemometer_count;
    uint16_t rain_count;
} sensor_data_t;


#endif /* INCLUDE_WEATHERBASE_H_ */