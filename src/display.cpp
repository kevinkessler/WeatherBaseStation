/**
 *  @filename   :   display.cpp
 *  @brief      :   ESP32 Weather Base Station display module
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
#include <SPI.h>
#include <time.h>
#include "Ticker.h"
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"
#include "Adafruit_I2CDevice.h"
#include "display.h"
#include "wifiwithmqtt.h"
#include "PanelBase.h"
#include "TemperaturePanel.h"
#include "HumidityPanel.h"
#include "HeaderPanel.h"
#include "ErrorPanel.h"
#include "RainPanel.h"
#include "BaroPanel.h"
#include "WindPanel.h"
#include "FT5206.h"

Adafruit_RA8875 tft = Adafruit_RA8875(CS, RST);

PanelList *first = NULL;
TemperaturePanel *tp1, *tp2;
HumidityPanel *hp1, *hp2;
HeaderPanel *headp;
ErrorPanel *ep;
RainPanel *rp;
BaroPanel *bp;
WindPanel *wp;

void resetTickerCallback(void);
void dataTickerCallback(void);
Ticker dataTimer(dataTickerCallback,300000); // 5 minutes
Ticker resetTimer(resetTickerCallback,300000);

static void waitForSignal(){
  uint16_t count=0;
  while(digitalRead(WAIT_PIN)!=1) {
    if(++count==0)          // Timeout of ~9ms, normally this takes ~720uS for large fonts
      break;
  }
}

void setArialFont(){
  tft.textMode();          // Resets font info, so don't run this after setting font
  tft.writeReg(0x21,0x20); // Font Control Register, turn on external CGROM, bit 5
  tft.writeReg(0x06,0x00); // Serial Flash CLK, SFCL=SystemClock
  tft.writeReg(0x2e,0x82); // Font Write Type, 80=32x32, 5-0=font to font pixels
  tft.writeReg(0x2f,0x91); // Serial Font Select, 80=GB2312 90=ASCII 8C=Unicode 84=GB12345 
  tft.writeReg(0x29,0x05); // Font line spacing
  tft.writeReg(0x05,0x28); // Serial Flash Rom Config
}

void setSmallArialFont(){
  tft.textMode();          // Resets font info, so don't run this after setting font
  tft.writeReg(0x21,0x20); // Font Control Register, turn on external CGROM, bit 5
  tft.writeReg(0x06,0x00); // Serial Flash CLK, SFCL=SystemClock
  tft.writeReg(0x2e,0x00); // Font Write Type, 80=32x32, 5-0=font to font pixels
  tft.writeReg(0x2f,0x91); // Serial Font Select, 80=GB2312 90=ASCII 8C=Unicode 84=GB12345 
  tft.writeReg(0x29,0x05); // Font line spacing
  tft.writeReg(0x05,0x28); // Serial Flash Rom Config
}

void printString(const char *s) {

  tft.writeCommand(RA8875_MRWC);
  while(*s!=0) {
      tft.writeData(*s);
      ++s;
      waitForSignal();
  }
}

void drawTransparentBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *bitmap) {

  tft.writeReg(0x58,x & 0xff);
  tft.writeReg(0x59,(x>>8));
  tft.writeReg(0x5A,y & 0xff);
  tft.writeReg(0x5B,(y>>8));
  tft.writeReg(0x5C,w & 0xff);
  tft.writeReg(0x5D, (w>>8));  
  tft.writeReg(0x5E,h & 0xff);
  tft.writeReg(0x5F,(h>>8));

  // White is the transparent color
  tft.writeReg(0x63,0xff);
  tft.writeReg(0x64,0xff);
  tft.writeReg(0x65,0xff); 

  tft.writeReg(0x51,0xc4);
  uint8_t reg=tft.readReg(0x50);
  reg=reg|0x80;
  tft.writeReg(0x50,reg);

  tft.writeCommand(RA8875_MRWC);


  digitalWrite(CS,LOW);

  SPI.beginTransaction(SPISettings(20000000UL, MSBFIRST, SPI_MODE0));
  SPI.transfer(RA8875_DATAWRITE);

 digitalWrite(DEBUG_PIN,HIGH);   
  SPI.writeBytes(bitmap,h*w*2);
   digitalWrite(DEBUG_PIN,LOW);   

  SPI.endTransaction();
  digitalWrite(CS,HIGH);

}

void drawThermometer(uint16_t x0, uint16_t y0, int8_t temperature) {
  tft.graphicsMode();

  drawTransparentBitmap(x0, y0, 27, 83, therm_bmp);
}

void drawCenteredArial(uint16_t centerx, uint16_t centery, int8_t value) {
  setArialFont();
  tft.textEnlarge(0);

  uint8_t textLength=16;
  if(abs(value)>99)
    textLength+=20;
  if(abs(value) > 9)
    textLength+=20;

  int16_t startx=centerx - textLength/2;
  if(value<0)                     // Space for the negitive sign
    startx-=13;
    if(startx<0)
      startx=0;  
  
  char buffer[5];
  tft.textSetCursor(startx,centery);
  itoa(value,buffer,10);
  printString(buffer);

}

void redrawBackgroundSection(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {

  if((h*w*2) > 200000) {
    Serial.println("Memory Allocation too large in redrawBackgroundSection");
    return;
  }

  uint8_t *bitmap=(uint8_t*)malloc(h*w*2);
  
  for(uint16_t n=0;n<h;n++) {
    memcpy(&bitmap[w*n*2],&background_bmp[((y+n)*800+x)*2],w*2);
  }

  drawTransparentBitmap(x,y,w,h,bitmap); 
  
  free(bitmap);
}

void display_panels() {
  ep = new ErrorPanel(&tft);
  ep->draw();

  if(first==NULL) {
    first = (PanelList *)malloc(sizeof(PanelList));
  }

  log("temperature","Temp 1 Create");
  tp1 = new TemperaturePanel(&tft, 0, 30, 75,false);
  tp1->draw();

  log("temperature","Temp 2 Create");
  tp2 = new TemperaturePanel(&tft, 549, 30, 75,true);
  tp2->draw();

  log("temperature","Hum 1 Create");
  hp1 = new HumidityPanel(&tft,0,261,50,false);
  hp1->draw();

  hp2 = new HumidityPanel(&tft,549,261,50,true);
  hp2->draw();

  rp = new RainPanel(&tft,255,30);
  rp -> draw();

  bp = new BaroPanel(&tft,255,160);
  bp->draw();

  wp = new WindPanel(&tft,255,330);
  wp->draw();

  first->p = tp1;
  PanelList *next = (PanelList *)malloc(sizeof(PanelList));
  first->next = next;

  next->p = tp2;
  PanelList *hnext = (PanelList *)malloc(sizeof(PanelList));
  next->next=hnext;
  hnext->p = hp1;

  next = (PanelList *)malloc(sizeof(PanelList));
  hnext->next = next;
  next->p = hp2;

  hnext = (PanelList *)malloc(sizeof(PanelList));
  next->next = hnext;
  hnext->p = rp;

  next = (PanelList *)malloc(sizeof(PanelList));
  hnext->next = next;
  next->p = bp;

  hnext = (PanelList *)malloc(sizeof(PanelList));  
  next->next=hnext;
  hnext->p = wp;
  hnext->next=NULL;

  headp = new HeaderPanel(&tft);
  headp->draw();


}

void background_panel() {
  drawTransparentBitmap(0,0,800,480,background_bmp);
}

void tftCTPTouch(uint16_t x, uint16_t y) {
  resetTimer.stop(); // reset timer that puts everything back to Daily extremes after 5 minutes
  resetTimer.start();

  if (first==NULL)
    return;

  PanelList *p = first;
  while(p!=NULL) {
    if(p->p->isClicked(x,y))
      break;
    p = p->next;
  }  

}

void displayData(float temperature, int32_t pressure, float humidity, float battery_millivolts, uint16_t direction, float anemometer, float rain, float roomTemp, float roomHum) {

  ep->clearMessage();
  
  dataTimer.stop();  //Reset the error timer
  dataTimer.start();

  
  tp1->setTemperature((int8_t)((9.0/5.0 * temperature) + 32.0 + 0.5));
  tp2->setTemperature((int8_t)((9.0/5.0 * roomTemp) + 32.0 + 0.5));
  
  hp1->setHumidity((uint8_t)(humidity+0.5));
  hp2->setHumidity((uint8_t)(roomHum +0.5));

  headp->setBatteryLevel(battery_millivolts/1000.0);

  rp->setRain(rain);

  float altitude=138.0;
  float altPressure=(float)(pressure)/(pow((1-altitude/44330.0),5.255));
  float hgPressure = altPressure/3386.39;

  bp->setBarometer(hgPressure);

  wp->setWind(anemometer,direction);
}

void setError(const char *errStr) {
  if(ep == NULL)
    return;

  ep->setMessage(errStr);
  log("errorpanel", errStr);
}

void displayLoop(void) {

  checkTouch();

  dataTimer.update();
  resetTimer.update();
}

void resetTickerCallback() {
  Serial.println("!!!!!Reset Timer!!!!!!!");

}

void dataTickerCallback() {
  Serial.println("!!!!!!!!Data Timeout!!!!!!!!!!");
  ep->setMessage("Error: No Data from station in 5 Minutes");
}

void initDisplay() {
  Serial.println("Display Start");
  if(!tft.begin(RA8875_800x480)) {
    Serial.println("Failed");
    return;
  }

  tft.displayOn(true);
  tft.GPIOX(true);
  tft.PWM1config(true,RA8875_PWM_CLK_DIV1024);
  tft.PWM1out(128);

  tft.fillScreen(RA8875_BLACK);

  pinMode(WAIT_PIN,INPUT);
  digitalWrite(DEBUG_PIN,0);
  ft5206_init();

  background_panel();
  display_panels();
  dataTimer.start();
  resetTimer.start();
}

void log(const char *system, const char *message) {
  logMessage(system, message);
}