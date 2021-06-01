/**
 *  @filename   :   TemperaturePanel.cpp
 *  @brief      :   ESP32 Weather Base Station Temperature Class
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
#include "TemperaturePanel.h"
#include "Adafruit_RA8875.h"
#include "display.h"
#include "InfluxDbQueries.h"


TemperaturePanel::TemperaturePanel(Adafruit_RA8875 *_tft, uint16_t _x, uint16_t _y, int8_t _current, bool _indoor) {

  tft = _tft;
  x_org = _x;
  y_org = _y;
  current = _current;
  high=-125;
  low=125;

  indoor = _indoor;  
  
  getDailyExtremes();
  refreshCount=0;

  highlow=DAILY;

  tempDirty = true;
  borderDirty = true;
  extremeDirty = true;
  
}

void TemperaturePanel::setTemperature(int8_t temperature) {

  if(refreshCount++ > 9) {
    refreshCount = 0;
    highlow=DAILY;
    extremeDirty = true;
  } else if(temperature == current) {
    if(extremeDirty)
      draw();
    return;
  }

  tempDirty = true;

  if (temperature < -99)
    temperature = -99;

  current = temperature;

  if(temperature < low) {
    extremeDirty = true;
    low = temperature;
  }

  if(temperature > high) {
    extremeDirty = true;
    high = temperature;
  }

  draw();

}

void TemperaturePanel::draw() {

  if(borderDirty) {
    tft->graphicsMode();
    tft->drawLine(x_org+25,y_org, x_org+TEMP_WIDTH-25,y_org,RA8875_YELLOW); // Top
    tft->drawLine(x_org,y_org+25, x_org,y_org+TEMP_HEIGTH,RA8875_YELLOW); // Left
    tft->drawLine(x_org+TEMP_WIDTH,y_org+25, x_org+TEMP_WIDTH,y_org+TEMP_HEIGTH,RA8875_YELLOW); // Right
    tft->drawLine(x_org,y_org+TEMP_HEIGTH, x_org+TEMP_WIDTH,y_org+TEMP_HEIGTH,RA8875_YELLOW); //Bottom
    tft->drawCurve(x_org+25, y_org+25, 25, 25,1,RA8875_YELLOW);
    tft->drawCurve(x_org+TEMP_WIDTH-25, y_org+25, 25, 25,2,RA8875_YELLOW);
    tft->textMode();
    tft->textTransparent(RA8875_WHITE);
    tft->textEnlarge(0);
    if (indoor) {
      tft->textSetCursor(x_org+(TEMP_WIDTH-162)/2, y_org+3);
      printString("Indoor ");
    }
    else{
      tft->textSetCursor(x_org+(TEMP_WIDTH-152)/2, y_org+3);
      printString("Outdoor ");
    }
    printString("Temperature");
    borderDirty = false;
  }

  if(tempDirty) {
    redrawBackgroundSection(x_org + 20, y_org + 40, TEMP_WIDTH - 50, 90);

    setArialFont();
    tft->textEnlarge(2);

    uint16_t xoffset = 50;
    uint16_t yoffset = 30;
    if(abs(current) < 10) {
      xoffset+=48;
    } else if(abs(current)> 99) {
      xoffset-=32;
    }

    if(current < 0) 
      xoffset-=32;
    
    tft->textSetCursor(x_org + xoffset, y_org + yoffset);

    char buffer[5];
    itoa(current,buffer,10);
    printString(buffer);

    drawThermometer(x_org+180,y_org+45);
    tempDirty = false;
  }
  
  if(extremeDirty) {
    drawExtremes();
    extremeDirty = false;
  }

  
}

void TemperaturePanel::drawThermometer(uint16_t x0, uint16_t y0) {
  tft->graphicsMode();

  drawTransparentBitmap(x0, y0, 27, 83, therm_bmp);

  // Temperature range -10 to 100
  int16_t hg_level = 55 - (current + 10) / 2;

  // Use a more compressed Thermometer for indoor
  if(indoor)
    hg_level = 55 - (current -60) *2;

  if(hg_level > 55)
    hg_level = 55;
  
  if(hg_level < 0)
    hg_level = 0;

  uint8_t *bmp = (uint8_t *)malloc((27 * (79-hg_level)*2));

  for(int n=hg_level;n<79; n++) {
    memcpy(&bmp[(n-hg_level)*27*2],&hg_bmp[n*27*2],27*2);
  }
  drawTransparentBitmap(x0, y0+hg_level+5, 27, 78-hg_level, bmp);
  free(bmp);
}

void TemperaturePanel::drawExtremes() {
  redrawBackgroundSection(x_org+1, y_org+TEMP_XTREME_YOFFSET, TEMP_WIDTH-1, 49);

  tft->textMode();
  tft->textTransparent(RA8875_WHITE);
  
  tft->textEnlarge(0);

  tft->textSetCursor(x_org+25,y_org+TEMP_XTREME_YOFFSET);
  printString("Low");
  tft->textSetCursor(x_org+TEMP_WIDTH-27-32,y_org+TEMP_XTREME_YOFFSET);
  printString("High");

  //Character Width = 8 Space =2
  setSmallArialFont();
  switch(highlow) {
    case DAILY:
      getDailyExtremes();
      tft->textSetCursor(x_org+(TEMP_WIDTH/2)-24,y_org+TEMP_XTREME_YOFFSET+15);
      printString("Daily");
      break;
    case WEEKLY:
      getExtendedExtremes(7);
      tft->textSetCursor(x_org+(TEMP_WIDTH/2)-29,y_org+TEMP_XTREME_YOFFSET+15);
      printString("Weekly");
      break;
    case MONTHLY:
      getExtendedExtremes(30);
      tft->textSetCursor(x_org+(TEMP_WIDTH/2)-34,y_org+TEMP_XTREME_YOFFSET+15);
      printString("Monthly");
      break;
    case YEARLY:
      getExtendedExtremes(365);
      tft->textSetCursor(x_org+(TEMP_WIDTH/2)-29,y_org+TEMP_XTREME_YOFFSET+15);
      printString("Yearly");
      break;      
    default:
      break;
  }
  
  drawCenteredArial((3*8)/2+25+x_org,y_org+TEMP_XTREME_YOFFSET+15,low);
  drawCenteredArial(x_org+(TEMP_WIDTH -27 -(4*8)/2),y_org+TEMP_XTREME_YOFFSET+15,high);
}

void TemperaturePanel::getDailyExtremes() {

  float newHigh,newLow;

  if(influxGetDailyHighLowTemp(indoor, &newHigh, &newLow))
    return;       //Some error occured

  // Round Up
  newHigh += 0.5;
  newLow += 0.5;

  high = newHigh;
  low = newLow;

  extremeDirty = true;
}

void TemperaturePanel::getExtendedExtremes(uint16_t timeLen) {
  float newHigh, newLow;
  getDailyExtremes();

  if(influxGetExtendedHighLowTemp(indoor, timeLen, &newHigh, &newLow))
    return;
  
  Serial.printf("newHigh %f newLow %f\n",newHigh,newLow);
  if(high > newHigh)
    newHigh=high;

  if(low < newLow)
    newLow = low;

  high = newHigh;
  low = newLow;

  extremeDirty = true;
}
bool TemperaturePanel::isClicked(uint16_t x, uint16_t y) {

  if((x>(x_org+TEMP_CLICK_MIN_X))&&(y>(y_org+TEMP_CLICK_MIN_Y))&&(x<(x_org+TEMP_CLICK_MAX_X))&&(y<(y_org+TEMP_XTREME_YOFFSET+TEMP_CLICK_MAX_Y))) {
    extremeDirty = true;
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

    draw();
    return true;
  }

  return false;
}


