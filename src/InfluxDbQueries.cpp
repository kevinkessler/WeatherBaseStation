/**
 *  @filename   :   InfluxDBQueries.cpp
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

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "InfluxDbQueries.h"
#include "display.h"
#include "wifiwithmqtt.h"
#include "time.h"

const char *dailyMinMaxQuery="/query?db=weather&q=SELECT%%20max%%28%%22%s%%22%%29%%20from%%20%%22station%%22%%20WHERE%%20time%%3E%%3Dnow%%28%%29-%dm%%3BSELECT%%20min%%28%%22%s%%22%%29%%20from%%20%%22station%%22%%20%%20WHERE%%20time%%3E%%3Dnow%%28%%29-%dm";
const char *extendedMinMaxQuery = "/query?db=weather&q=SELECT%%20max%%28%s%%29%%20from%%20two_year.hourly_rollup%%20WHERE%%20time%%3E%%3Dnow%%28%%29-%dd%%3BSELECT%%20min%%28%s%%29%%20from%%20two_year.hourly_rollup%%20WHERE%%20time%%3E%%3Dnow%%28%%29-%dd";
const char *dailyRainQuery="/query?db=weather&q=SELECT%20sum%28%22rain%22%29%20from%20%22station%22%20WHERE%20time%3E%3Dnow%28%29-24h";
const char *extendedRainQuery="/query?db=weather&q=SELECT%%20sum%%28rain%%29%%20from%%20two_year.hourly_rollup%%20WHERE%%20time%%3E%%3Dnow%%28%%29-%dd%%20AND%%20time%%20%%3C%%20now%%28%%29-24h";
const char *avePressureQuery="/query?db=weather&q=SELECT%20mean%28pressureHg%29%20from%20station%20WHERE%20time%3E%3Dnow%28%29-2h";

extern char mqttServer[MQTT_SERVER_LENGTH];

uint16_t doMaxMinHttpQuery(const char *url, float *high, float *low) {
  HTTPClient hc;

  hc.begin(url);

  int rc=hc.GET();

  if(rc == 200) {

    String payload=hc.getString();
    StaticJsonDocument<768> doc;
    deserializeJson(doc, payload);

    float min=doc["results"][1]["series"][0]["values"][0][1];
    float max=doc["results"][0]["series"][0]["values"][0][1];
    
    *low =min;
    *high=max;
  
  }

  hc.end();

  return rc;

}

uint16_t influxGetDailyMaxMin(const char *column, float *high, float *low) {
  char url[256];
  char uri[256];

  struct  tm dt;
  uint16_t minutes=0;

  // Issue: when starting from a power up or reset, getLocalTime always times out
  // on each panel as it is built. Even putting displayInit in the main loop, does
  // not solve the problem
  if(getLocalTime(&dt,1000)) {
    minutes = dt.tm_hour * 60 + dt.tm_min;
  } else {
    minutes = 1440;
  }

  sprintf(uri,dailyMinMaxQuery,column,minutes,column,minutes);
  sprintf(url,"http://%s:8086%s",mqttServer,uri);
  
  return doMaxMinHttpQuery(url, high, low);

}

uint16_t influxGetExtendedMaxMin(const char *maxCol, const char *minCol, uint16_t timeLen, float *high, float *low) {
  char url[256];
  char uri[256];
  
  sprintf(uri,extendedMinMaxQuery,maxCol,timeLen,minCol,timeLen);
  sprintf(url,"http://%s:8086%s",mqttServer,uri);

  return doMaxMinHttpQuery(url,high,low);
}

uint8_t influxGetExtendedHighLowTemp(bool indoor, uint16_t timeLen, float *high, float *low)
{
  uint16_t rc=0;
  if(indoor) {
    rc=influxGetExtendedMaxMin("max_room_temp","min_room_temp",timeLen,high,low);
  } else {
    rc=influxGetExtendedMaxMin("max_temp","min_temp",timeLen,high,low);
  }

  uint8_t retval=0;
  if(rc!=200) {
    retval = 1;
    char error[70];
    sprintf(error,"Extended %s HighLow Temperature Query returned %d",(indoor) ? "Indoor" : "Outdoor", rc);
    setError(error);
  }

  return retval;
}

uint8_t influxGetDailyHighLowTemp(bool indoor, float *high, float *low) {

  uint16_t rc=0;

  if(indoor)
    rc = influxGetDailyMaxMin("room_temp",high,low);
  else  
    rc = influxGetDailyMaxMin("temperature",high,low);
  
  uint8_t retval=0;
  if(rc!=200) {
    retval = 1;
    char error[70];
    sprintf(error,"Daily %s HighLow Temperature Query returned %d",(indoor) ? "Indoor" : "Outdoor", rc);
    setError(error);
  }

  return retval;
}

uint8_t influxGetDailyHighLowHum(bool indoor,float *high, float *low) {
  uint16_t rc=0;

  if(indoor)
    rc = influxGetDailyMaxMin("room_hum",high,low);
  else  
    rc = influxGetDailyMaxMin("humidity",high,low);

  uint8_t retval=0;
  if(rc!=200) {
    retval = 1;
    char error[70];
    sprintf(error,"Daily %s HighLow Humidity Query returned %d",(indoor) ? "Indoor" : "Outdoor", rc);
    setError(error);
  }

  return retval;
}

uint8_t influxGetExtendedHighLowHum(bool indoor, uint16_t timeLen, float *high, float *low) {
  uint16_t rc=0;
  if(indoor) {
    rc=influxGetExtendedMaxMin("max_room_hum","min_room_hum",timeLen,high,low);
  } else {
    rc=influxGetExtendedMaxMin("max_humidity","min_humidity",timeLen,high,low);
  }

  uint8_t retval=0;
  if(rc!=200) {
    retval = 1;
    char error[70];
    sprintf(error,"Extended %s HighLow Humidity Query returned %d",(indoor) ? "Indoor" : "Outdoor", rc);
    setError(error);
  }

  return retval;
}

uint8_t influxGetDailyHighLowPress(float *high, float *low) {
  uint16_t rc=influxGetDailyMaxMin("pressureHg", high, low);
  uint8_t retval =0;
  if(rc != 200) {
    retval =1;
    char error[70];
    sprintf(error,"Daily HighLow Pressure Query returned %d", rc);
    setError(error);    
  }

  return retval;

}

uint8_t influxGetExtendedHighLowPress(uint16_t timeLen, float *high, float *low) {
  uint16_t rc=influxGetExtendedMaxMin("max_pressureHg","min_pressureHg", timeLen, high, low);

  uint8_t retval=0;
  if(rc != 200) {
    retval =1;
    char error[70];
    sprintf(error,"Extended HighLow Pressure Query returned %d", rc);
    setError(error);    
  }
  return retval;
}

uint8_t influxGetAveragePressure(float *ave) {
  char url[256];

  sprintf(url,"http://%s:8086%s",mqttServer,avePressureQuery);
  
  HTTPClient hc;

  hc.begin(url);

  uint8_t retval = 0;
  int rc=hc.GET();

  if(rc == 200) {

    String payload=hc.getString();
    StaticJsonDocument<768> doc;
    deserializeJson(doc, payload);

    *ave=doc["results"][0]["series"][0]["values"][0][1];
    
    Serial.printf("Ave Pressure %f\n",*ave);

  } else {
    retval = 1;
    char error[70];
    sprintf(error,"Average Pressure Query returned %d", rc);
    setError(error);
  }


  hc.end();
  return retval;

}

uint8_t influxGetDailyRain(float *rain) {
  char url[256];
  sprintf(url,"http://%s:8086%s",mqttServer,dailyRainQuery);

  HTTPClient hc;

  hc.begin(url);

  uint8_t retval = 0;
  int rc=hc.GET();

  if(rc == 200) {

    String payload=hc.getString();
    StaticJsonDocument<768> doc;
    deserializeJson(doc, payload);

    *rain=doc["results"][0]["series"][0]["values"][0][1];
    
    Serial.printf("Rain %f\n",*rain);

  } else {
    retval = 1;
    char error[70];
    sprintf(error,"Daily Rain Query returned %d", rc);
    setError(error);
  }


  hc.end();

  return retval;
}

uint8_t influxGetExtendedRain(uint16_t timeLen,float *rain) {
  char url[256];
  char uri[256];

  uint8_t retval = influxGetDailyRain(rain);
  if(timeLen <2)
    return retval;

  if(retval !=0)
    return retval;

  sprintf(uri,extendedRainQuery,timeLen);
  sprintf(url,"http://%s:8086%s",mqttServer,uri);
  
  HTTPClient hc;

  hc.begin(url);
  int rc=hc.GET();
  float newRain=0.0;

  if(rc == 200) {

    String payload=hc.getString();
    StaticJsonDocument<768> doc;
    deserializeJson(doc, payload);

    newRain=doc["results"][0]["series"][0]["values"][0][1];
    
    Serial.printf("newRain %f\n",newRain);
    *rain+=newRain;

  } else {
    retval = 1;
    char error[70];
    sprintf(error,"Daily Rain Query returned %d", rc);
    setError(error);
  }


  hc.end();

  return retval;
}