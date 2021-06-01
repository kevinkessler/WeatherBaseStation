/**
 *  @filename   :   WindPanel.cpp
 *  @brief      :   ESP32 Weather Base Station Wind Display Class
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
#include "WindPanel.h"
#include "Adafruit_RA8875.h"
#include "display.h"

WindPanel::WindPanel(Adafruit_RA8875 *_tft, uint16_t _x, uint16_t _y) {
  tft = _tft;
  x_org = _x;
  y_org = _y;
  current = 0.0;
  windDir = 293;
  windDirty = true;
  borderDirty = true;
}

void WindPanel::draw() {

  if(borderDirty) {
    tft->graphicsMode();
    tft->drawLine(x_org,y_org, x_org,y_org+WIND_HEIGTH-25,RA8875_YELLOW); // Left
    tft->drawLine(x_org+WIND_WIDTH,y_org, x_org+WIND_WIDTH,y_org+WIND_HEIGTH-25,RA8875_YELLOW); // Right
    tft->drawLine(x_org+25,y_org+WIND_HEIGTH, x_org+WIND_WIDTH-25,y_org+WIND_HEIGTH,RA8875_YELLOW); //Bottom
    tft->drawCurve(x_org+25, y_org+WIND_HEIGTH-25, 25, 25,0,RA8875_YELLOW);
    tft->drawCurve(x_org+WIND_WIDTH-25, y_org+WIND_HEIGTH-25, 25, 25,3,RA8875_YELLOW);

    tft->textMode();
    tft->textTransparent(RA8875_WHITE);
    tft->textEnlarge(0);
    
    tft->textSetCursor(x_org+(WIND_WIDTH-40)/2, y_org+3);
    printString("Wind");

    borderDirty = false;
  }

  if(windDirty) {
    redrawBackgroundSection(x_org+ 25, y_org+30, WIND_WIDTH-30, WIND_HEIGTH - 42);
    //tft->drawRect(x_org+ 25, y_org+30, WIND_WIDTH-30, WIND_HEIGTH - 42, RA8875_GREEN);

    tft->textMode();
    setArialFont();
    tft->textEnlarge(1);
    tft->textTransparent(RA8875_WHITE);
    tft->textSetCursor(x_org+35,y_org+30);


    char buffer[4];
    if(current < 10) {
      sprintf(buffer,"%2.1f",current);
    } else if(current <100) {
      sprintf(buffer,"%2.0f",current);
    } 

    printString(buffer);

    tft->textEnlarge(0);
    switch(windDir) {
      case 0:
        tft->textSetCursor(x_org+192, y_org+50);
        printString("N");
        break;
      case 23:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("NNE");
        break;
      case 45:
        tft->textSetCursor(x_org+176, y_org+50);
        printString("NE");
        break;
      case 68:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("ENE");
        break;
      case 90:
        tft->textSetCursor(x_org+192, y_org+50);
        printString("E");
        break;
      case 113:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("ESE");
        break;
      case 135:
        tft->textSetCursor(x_org+176, y_org+50);
        printString("SE");
        break;
      case 158:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("SSE");
        break;
      case 180:
        tft->textSetCursor(x_org+192, y_org+50);
        printString("S");
        break;
      case 203:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("SSW");
        break;
      case 225:
        tft->textSetCursor(x_org+176, y_org+50);
        printString("SW");
        break;
      case 248:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("WSW");
        break;
      case 270:
        tft->textSetCursor(x_org+192, y_org+50);
        printString("W");
        break;
      case 293:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("WNW");
        break;
      case 315:
        tft->textSetCursor(x_org+176, y_org+50);
        printString("NW");
        break;
      case 338:
        tft->textSetCursor(x_org+160, y_org+50);
        printString("NNW");
        break;
      default:
        tft->textSetCursor(x_org+192, y_org+50);
        printString("X");
        break;
    }

    windDirty = false;
  }
  
}

bool WindPanel::isClicked(uint16_t x, uint16_t y) {
  return false;
}

void WindPanel::setWind(float wind, uint16_t dir) {

  if ((wind == current) && (dir == windDir))
    return;

  current= wind;
  if(current < 0) {
    current = 0;
  }

  windDir=dir;

  windDirty = true;

  draw();

}