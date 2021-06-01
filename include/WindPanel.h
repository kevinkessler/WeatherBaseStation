/**
 *  @filename   :   WindPanel.h
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
#ifndef INCLUDE_WINDPANEL_H_
#define INCLUDE_WINDPANEL_H_

#include <Arduino.h>
#include "Adafruit_RA8875.h"
#include "PanelBase.h"

#define WIND_WIDTH 289
#define WIND_HEIGTH 120

class WindPanel: virtual public PanelBase {
  public:
    WindPanel(Adafruit_RA8875 *tft, uint16_t x, uint16_t y);
    void draw(void);
    void setWind(float wind, uint16_t _dir);
    bool isClicked(uint16_t x, uint16_t y) override;

  private:
    Adafruit_RA8875 *tft;
    uint16_t x_org;
    uint16_t y_org;
    float current;
    uint16_t windDir;

    bool windDirty;
    bool borderDirty;
};

#endif /* INCLUDE_WINDPANEL_H_ */