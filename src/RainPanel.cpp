/**
 *  @filename   :   RainPanel.cpp
 *  @brief      :   ESP32 Weather Base Station Rain Gauge Display Class
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
#include "RainPanel.h"
#include "Adafruit_RA8875.h"
#include "display.h"
#include "InfluxDbQueries.h"
#include "time.h"

RainPanel::RainPanel(Adafruit_RA8875 *_tft, uint16_t _x, uint16_t _y) {
  tft = _tft;
  x_org = _x;
  y_org = _y;
  current = 0.0;
  getDailyRain();
  refreshCount=0;
  rainPeriod=DAILY;

  rainDirty = true;
  borderDirty = true;
}

void RainPanel::draw() {

  if(borderDirty) {
    tft->graphicsMode();
    tft->drawLine(x_org+25,y_org, x_org+RAIN_WIDTH-25,y_org,RA8875_YELLOW); // Top
    tft->drawLine(x_org,y_org+25, x_org,y_org+RAIN_HEIGTH,RA8875_YELLOW); // Left
    tft->drawLine(x_org+RAIN_WIDTH,y_org+25, x_org+RAIN_WIDTH,y_org+RAIN_HEIGTH,RA8875_YELLOW); // Right
    tft->drawLine(x_org,y_org+RAIN_HEIGTH, x_org+RAIN_WIDTH,y_org+RAIN_HEIGTH,RA8875_YELLOW); //Bottom
    tft->drawCurve(x_org+25, y_org+25, 25, 25,1,RA8875_YELLOW);
    tft->drawCurve(x_org+RAIN_WIDTH-25, y_org+25, 25, 25,2,RA8875_YELLOW);

    drawTransparentBitmap(x_org+210,y_org+45,50,50,rain);

    tft->textMode();
    tft->textTransparent(RA8875_WHITE);
    tft->textEnlarge(0);
    tft->textSetCursor(x_org+(RAIN_WIDTH - 100)/2, y_org+3);
    printString("Rain Gauge");
    borderDirty = false;
  }

  if(rainDirty) {
    struct tm dt;
    redrawBackgroundSection(x_org+ 10, y_org+40, RAIN_WIDTH-90, RAIN_HEIGTH - 42);

    tft->textMode();

    setSmallArialFont();
    tft->textTransparent(RA8875_WHITE);
    tft->textEnlarge(0);

    switch(rainPeriod) {
      case DAILY:
        getDailyRain();
        tft->textSetCursor(x_org+(RAIN_WIDTH/2)-(7*8/2),y_org+100);
        printString("24 Hour");
        break;
      case WEEKLY:
        getExtendedRain(7);
        tft->textSetCursor(x_org+(RAIN_WIDTH/2)-(6*8/2),y_org+100);
        printString("7 Days");
        break;
      case MONTHLY:
        getLocalTime(&dt);
        getExtendedRain(dt.tm_mday);
        tft->textSetCursor(x_org+(RAIN_WIDTH/2)-(13*8/2),y_org+100);
        printString("Month to Date");
        break;
      case YEARLY:
        getLocalTime(&dt);
        getExtendedRain(dt.tm_yday);
        tft->textSetCursor(x_org+(RAIN_WIDTH/2)-(12*8/2),y_org+100);
        printString("Year to Date");
        break;      
      default:
        break;
    }

    setArialFont();
    tft->textEnlarge(1);
    tft->textSetCursor(x_org+30,y_org+30);

    char buffer[6];
    if(current < 10) {
      sprintf(buffer,"%3.2f\"",current);
    } else if(current <100) {
      sprintf(buffer,"%3.1f\"",current);
    } else {
      sprintf(buffer, "%3.0f\"",current);
    }
    printString(buffer);

    rainDirty = false;
  }

}

void RainPanel::setRain(float rain) {
  if(rain!=0.0) {
    rainDirty=true;
    draw();
  } else if(++refreshCount>9) {
    refreshCount=0;
    rainPeriod=DAILY;
    rainDirty=true;
    draw();
  }

}

bool RainPanel::isClicked(uint16_t x, uint16_t y) {

  if((x>(x_org+RAIN_CLICK_MIN_X))&&(y>(y_org+RAIN_CLICK_MIN_Y))&&(x<(x_org+RAIN_CLICK_MAX_X))&&(y<(y_org+RAIN_CLICK_MAX_Y))) {

  switch(rainPeriod) {
    case DAILY:
      rainPeriod=WEEKLY;
      break;
    case WEEKLY:
      rainPeriod=MONTHLY;
      break;
    case MONTHLY:
      rainPeriod=YEARLY;
      break;
    default:
      rainPeriod=DAILY;
      break;
  }

    rainDirty = true;
    
    refreshCount = 0;
    draw();
    return true;
  }

  return false;
}

void RainPanel::getDailyRain() {
  influxGetDailyRain(&current);

  rainDirty = true;
}

void RainPanel::getExtendedRain(uint16_t timeLen) {
  influxGetExtendedRain(timeLen,&current);

  rainDirty=true;
}