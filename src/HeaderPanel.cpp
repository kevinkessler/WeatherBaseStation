/**
 *  @filename   :   HeaderPanel.cpp
 *  @brief      :   ESP32 Weather Base Station Header Display Class
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
#include "weatherbase.h"
#include "HeaderPanel.h"
#include "Adafruit_RA8875.h"
#include "display.h"
#include "time.h"

HeaderPanel::HeaderPanel(Adafruit_RA8875 *_tft) {
  tft = _tft;

  configTime(GMT_OFFSET_SECS, DAYLIGHT_OFFSET_SECS, "pool.ntp.org");
  strcpy(timeBuffer,"00:00");
  strcpy(dateBuffer, "00/00/00");
  battery_level = 4.2;

}

void HeaderPanel::draw() {
  fillDateTimeBuffers();
  tft->fillRect(0,0,799,20,RA8875_WHITE);

  tft->textMode();
  tft->textEnlarge(0);
  tft->textTransparent(RA8875_BLACK);

  if(strlen(timeBuffer)==5)
    tft->textSetCursor(380,1);
  else
    tft->textSetCursor(390,1);

  printString(timeBuffer);

  tft->textSetCursor(710,1);
  printString(dateBuffer);

  tft->graphicsMode();
  float level = battery_level;
  if (level > 4.2)
    level = 4.2;

  if (level < 3.0)
    level = 3.0;
  
  uint8_t offset = uint8_t(((level - 3.0) * 100.0)/6.0);

  uint16_t color = RA8875_GREEN;
  if(level < 3.9)
    color = RA8875_YELLOW;
  if (level < 3.4)
    color = RA8875_RED;

  tft->fillRect(48-offset,4,offset +1, 12, color);

  drawTransparentBitmap(25,0,28,20,battery);

}

void HeaderPanel::setBatteryLevel(float level) {
  battery_level = level;
  if(level < 3.2) {
    char errStr[70];
    sprintf(errStr,"Voltage Level %f is below 3.2 V",level);
    setError(errStr);
  }
  draw();
}

void HeaderPanel::fillDateTimeBuffers() {
  struct tm dt;

  if(getLocalTime(&dt)) {
    strftime(timeBuffer,6,"%I:%M",&dt);
    strftime(dateBuffer,9,"%D",&dt);
  } else {
    strcpy(timeBuffer,"XX:XX");
    strcpy(dateBuffer, "XX/XX/XX");
    char errStr[70];
    sprintf(errStr,"Error getting local time");
    setError(errStr);
  }
}