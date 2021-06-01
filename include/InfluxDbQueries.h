/**
 *  @filename   :   InfluxDBQueries.h
 *  @brief      :   ESP32 Weather Base Station InfluxDB Queries Class
 *
 *  @author     :   Kevin Kessler
 *
 * Copyright (C) 2021 Kevin Kessler
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

#ifndef INCLUDE_INFLUXDBQUERIES_H_
#define INCLUDE_INFLUXDBQUERIEs_H_

uint8_t influxGetDailyHighLowTemp(bool indoor, float *high, float *low);
uint8_t influxGetExtendedHighLowTemp(bool indoor, uint16_t timeLen, float *high, float *low);
uint8_t influxGetDailyRain(float *rain);
uint8_t influxGetExtendedRain(uint16_t timeLen,float *rain);
uint8_t influxGetDailyHighLowHum(bool indoor,float *high, float *low);
uint8_t influxGetExtendedHighLowHum(bool indoor, uint16_t timeLen, float *high, float *low);
uint8_t influxGetDailyHighLowPress(float *high, float *low);
uint8_t influxGetExtendedHighLowPress(uint16_t timeLen, float *high, float *low);
uint8_t influxGetAveragePressure(float *ave);

#endif /* INCLUDE_INFLUXDBQUERIEs_H_ */