/**
 *  @filename   :   HumidityPanel.cpp
 *  @brief      :   ESP32 Weather Base Station Humidity Display Class
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
#include "HumidityPanel.h"
#include "Adafruit_RA8875.h"
#include "display.h"
#include "InfluxDbQueries.h"

HumidityPanel::HumidityPanel(Adafruit_RA8875 *_tft, uint16_t _x, uint16_t _y, int8_t _current, bool _indoor) {
  tft = _tft;
  x_org = _x;
  y_org = _y;
  current = 50;
  low = _current;
  high = _current;
  indoor = _indoor;

  highlow=DAILY;
  getDailyExtremes();

  refreshCount=0;
  humDirty = true;
  borderDirty = true;
  extremeDirty = true;
}

void HumidityPanel::draw() {

  if(borderDirty) {
    tft->graphicsMode();
    tft->drawLine(x_org,y_org, x_org,y_org+HUM_HEIGTH-25,RA8875_YELLOW); // Left
    tft->drawLine(x_org+HUM_WIDTH,y_org, x_org+HUM_WIDTH,y_org+HUM_HEIGTH-25,RA8875_YELLOW); // Right
    tft->drawLine(x_org+25,y_org+HUM_HEIGTH, x_org+HUM_WIDTH-25,y_org+HUM_HEIGTH,RA8875_YELLOW); //Bottom
    tft->drawCurve(x_org+25, y_org+HUM_HEIGTH-25, 25, 25,0,RA8875_YELLOW);
    tft->drawCurve(x_org+HUM_WIDTH-25, y_org+HUM_HEIGTH-25, 25, 25,3,RA8875_YELLOW);

    tft->textMode();
    tft->textTransparent(RA8875_WHITE);
    tft->textEnlarge(0);
    if (indoor) {
      tft->textSetCursor(x_org+(HUM_WIDTH-154)/2, y_org+3);
      printString("Indoor ");
    }
    else{
      tft->textSetCursor(x_org+(HUM_WIDTH-160)/2, y_org+3);
      printString("Outdoor ");
    }
    printString("Humidity");
    borderDirty = false;
  }

  if(humDirty) {
    uint16_t xoffset = 60;
    uint16_t yoffset = 30;   
    
    redrawBackgroundSection(x_org + xoffset - 20, y_org + yoffset, HUM_WIDTH - 75, 75);
    setArialFont();
    tft->textEnlarge(1);
    

 
    if(abs(current) < 10) {
      xoffset+=38;
    }

    
    tft->textSetCursor(x_org + xoffset, y_org + yoffset);

    char buffer[5];
    itoa(current,buffer,10);
    printString(buffer);
    printString("%");

    humDirty = false;
  }
  
  if(extremeDirty) {
    drawExtremes();
    extremeDirty = false;
  }
}

void HumidityPanel::drawExtremes() {

  redrawBackgroundSection(x_org+ 1, y_org+HUM_XTREME_YOFFSET, HUM_WIDTH-1, 49);

  tft->textMode();
  tft->textTransparent(RA8875_WHITE);
  
  tft->textEnlarge(0);

  tft->textSetCursor(x_org+25,y_org+HUM_XTREME_YOFFSET);
  printString("Low");
  tft->textSetCursor(x_org+HUM_WIDTH-27-32,y_org+HUM_XTREME_YOFFSET);
  printString("High");

  //Character Width = 8 Space =2
  setSmallArialFont();
  switch(highlow) {
    case DAILY:
      getDailyExtremes();
      tft->textSetCursor(x_org+(HUM_WIDTH/2)-24,y_org+HUM_XTREME_YOFFSET+15);
      printString("Daily");
      break;
    case WEEKLY:
      getExtendedExtremes(7);
      tft->textSetCursor(x_org+(HUM_WIDTH/2)-29,y_org+HUM_XTREME_YOFFSET+15);
      printString("Weekly");
      break;
    case MONTHLY:
      getExtendedExtremes(30);
      tft->textSetCursor(x_org+(HUM_WIDTH/2)-34,y_org+HUM_XTREME_YOFFSET+15);
      printString("Monthly");
      break;
    case YEARLY:
      getExtendedExtremes(365);
      tft->textSetCursor(x_org+(HUM_WIDTH/2)-29,y_org+HUM_XTREME_YOFFSET+15);
      printString("Yearly");
      break;      
    default:
      break;
  }
  
  drawCenteredArial((3*8)/2+25+x_org,y_org+HUM_XTREME_YOFFSET+15,low);
  drawCenteredArial(x_org+(HUM_WIDTH -27 -(4*8)/2),y_org+HUM_XTREME_YOFFSET+15,high);
}

bool HumidityPanel::isClicked(uint16_t x, uint16_t y) {

  if((x>(x_org+HUM_CLICK_MIN_X))&&(y>(y_org+HUM_CLICK_MIN_Y))&&(x<(x_org+HUM_CLICK_MAX_X))&&(y<(y_org+HUM_XTREME_YOFFSET+HUM_CLICK_MAX_Y))) {
    Serial.println("Clicked");
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

    refreshCount = 0;
    draw();
    return true;
  }

  return false;
}

void HumidityPanel::setHumidity(uint8_t humidity) {

  if(refreshCount++ > 9) {
    refreshCount = 0;
    extremeDirty = true;
    highlow=DAILY;
  } else if (humidity == current) {
    if(extremeDirty)
      draw();
    return;
  }

  current= humidity;
  if(current > 99) {
    current = 99;
  }

  humDirty = true;
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

void HumidityPanel::getDailyExtremes() {

  float newHigh,newLow;

  if(influxGetDailyHighLowHum(indoor, &newHigh, &newLow))
    return;       //Some error occured

  // Round Up
  newHigh += 0.5;
  newLow += 0.5;

  high = newHigh;
  low = newLow;

  extremeDirty = true;
}

void HumidityPanel::getExtendedExtremes(uint16_t timeLen) {
  float newHigh, newLow;
  getDailyExtremes();

  if(influxGetExtendedHighLowHum(indoor, timeLen, &newHigh, &newLow))
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