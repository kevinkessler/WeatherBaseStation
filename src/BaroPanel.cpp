/**
 *  @filename   :   BaroPanel.cpp
 *  @brief      :   ESP32 Weather Base Station Barometer Display Class
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
#include "BaroPanel.h"
#include "Adafruit_RA8875.h"
#include "display.h"
#include "InfluxDbQueries.h"

BaroPanel::BaroPanel(Adafruit_RA8875 *_tft, uint16_t _x, uint16_t _y) {
  tft = _tft;
  x_org = _x;
  y_org = _y;
  current = 29.7312;
  low = current;
  high = current;

  baroDir = BARO_STEADY;
  getDailyExtremes();
  getAveragePressure();
  averagePoll=0;

  highlow=DAILY;

  baroDirty = true;
  borderDirty = true;
  extremeDirty = true;
}

void BaroPanel::draw() {

  if(borderDirty) {
    tft->graphicsMode();
    tft->drawRect(x_org,y_org,BARO_WIDTH+1,BARO_HEIGTH,RA8875_YELLOW);

    tft->textMode();
    tft->textTransparent(RA8875_WHITE);
    tft->textEnlarge(0);
    tft->textSetCursor(x_org+(BARO_WIDTH - 90)/2, y_org+3);
    printString("Barometer");
    borderDirty = false;
  }

  if(baroDirty) {
    if(abs(current-average) < 0.005) {
      baroDir=BARO_STEADY;
    } else if(current > average) {
      baroDir=BARO_RISING;
    } else {
      baroDir=BARO_FALLING;
    }

    char buffer[6];
    sprintf(buffer,"%4.2f", current);

    uint8_t y_offset=30;
    redrawBackgroundSection(x_org+35,y_org+y_offset,225,85);

    tft->textMode();
    setArialFont();
    tft->textEnlarge(1);
    tft->textSetCursor(x_org+35,y_org+y_offset);
    printString(buffer);

    switch(baroDir) {
      case BARO_RISING:
        drawTransparentBitmap(x_org+210,y_org+y_offset+12,43,50, up_arrow);
        break;
      case BARO_FALLING:
        drawTransparentBitmap(x_org+210,y_org+y_offset+12,43,50, down_arrow);
        break;
      case BARO_STEADY:
        drawTransparentBitmap(x_org+210,y_org+y_offset+12,43,50, steady);
        break;
    }

    baroDirty = false;
  }

  if(extremeDirty) {
    drawExtremes();
    extremeDirty = false;
  }
}

void BaroPanel::setBarometer(float baro) {
  if(averagePoll>9) {             // Refresh average pressure for Rise Fall indicator
      getAveragePressure();
      extremeDirty=true;
      baroDirty=true;
      averagePoll=0;
      highlow=DAILY;
  } else if (baro == current) {
    if(baroDirty)
      draw();
    return;
  }

  if (baro < 0.0)
    baro = 0.0;

  if (baro != current) {
    current = baro;
    baroDirty = true;

    if(current < low) {
      extremeDirty = true;
      low = current;
    }

    if(current > high) {
      extremeDirty = true;
      high = current;
    }
    draw();
  }

}

void BaroPanel::drawExtremes() {
  redrawBackgroundSection(x_org+ 1, y_org+BARO_XTREME_YOFFSET, BARO_WIDTH-1, 49);

  tft->textMode();
  tft->textTransparent(RA8875_WHITE);
  
  tft->textEnlarge(0);

  tft->textSetCursor(x_org+34,y_org+BARO_XTREME_YOFFSET);
  printString("Low");
  tft->textSetCursor(x_org+BARO_WIDTH-32-32,y_org+BARO_XTREME_YOFFSET);
  printString("High");
  
  setSmallArialFont();
  tft->textEnlarge(0);
  switch(highlow) {
    case DAILY:
      getDailyExtremes();
      tft->textSetCursor(x_org+(BARO_WIDTH/2)-24,y_org+BARO_XTREME_YOFFSET+15);
      printString("Daily");
      break;
    case WEEKLY:
      getExtendedExtremes(7);
      tft->textSetCursor(x_org+(BARO_WIDTH/2)-29,y_org+BARO_XTREME_YOFFSET+15);
      printString("Weekly");
      break;
    case MONTHLY:
      getExtendedExtremes(30);
      tft->textSetCursor(x_org+(BARO_WIDTH/2)-34,y_org+BARO_XTREME_YOFFSET+15);
      printString("Monthly");
      break;
    case YEARLY:
      getExtendedExtremes(365);
      tft->textSetCursor(x_org+(BARO_WIDTH/2)-29,y_org+BARO_XTREME_YOFFSET+15);
      printString("Yearly");
      break;      
    default:
      break;
  }

  char buffer[6];
  setArialFont();
  sprintf(buffer,"%4.2f",low);
  tft->textSetCursor(x_org+5,y_org+BARO_XTREME_YOFFSET+15);
  printString(buffer);

  sprintf(buffer,"%4.2f",high);
  tft->textSetCursor(x_org+BARO_WIDTH-90,y_org+BARO_XTREME_YOFFSET+15);
  printString(buffer);

}

bool BaroPanel::isClicked(uint16_t x, uint16_t y) {

  if((x>(x_org+BARO_CLICK_MIN_X))&&(y>(y_org+BARO_CLICK_MIN_Y))&&(x<(x_org+BARO_CLICK_MAX_X))&&(y<(y_org+BARO_CLICK_MAX_Y))) {

    switch(highlow) {
      case DAILY:
        highlow=WEEKLY;
        break;
      case WEEKLY:
        highlow=MONTHLY;
        break;
      case MONTHLY:
        highlow=YEARLY;
        break;
      default:
        highlow=DAILY;
        break;
    }

    extremeDirty = true;

    draw();
    return true;
  }

  return false;
}

void BaroPanel::getDailyExtremes() {

  float newHigh,newLow;

  if(influxGetDailyHighLowPress(&newHigh, &newLow))
    return;       //Some error occured

  // Round Up
  newHigh += 0.005;
  newLow += 0.005;

  high = newHigh;
  low = newLow;

  extremeDirty = true;
}

void BaroPanel::getExtendedExtremes(uint16_t timeLen) {
  float newHigh, newLow;
  getDailyExtremes();

  if(influxGetExtendedHighLowPress(timeLen, &newHigh, &newLow))
    return;
  
  if(high > newHigh)
    newHigh=high;

  if(low < newLow)
    newLow = low;

  high = newHigh;
  low = newLow;

  extremeDirty = true;
}

void BaroPanel::getAveragePressure(){
  float avgPress;
  if(influxGetAveragePressure(&avgPress))
    return;     // An error has occured

  average=avgPress;
}