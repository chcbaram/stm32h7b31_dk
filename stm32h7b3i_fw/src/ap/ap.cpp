/*
 * ap.cpp
 *
 *  Created on: 2020. 7. 10.
 *      Author: Baram
 */




#include "ap.h"






void apInit(void)
{
  hwInit();


  cmdifOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;
  uint32_t pre_time_fps;
  uint32_t fps_time;
  uint32_t fps;
  uint16_t x = 0;
  uint16_t y = 0;
  bool fps_update = true;


  ledOn(_DEF_LED1);
  ledOff(_DEF_LED2);

  pre_time = millis();
  while(1)
  {
    cmdifMain();

    if (millis()-pre_time >= 500)
    {
      pre_time = millis();

      ledToggle(_DEF_LED1);
      ledToggle(_DEF_LED2);
      fps_update = true;
    }

    if (lcdDrawAvailable() > 0)
    {
      lcdClear(black);

      if (fps_update)
      {
        fps_time = millis()-pre_time_fps;
        fps      = 1000/(millis()-pre_time_fps);
        fps_update = false;
      }
      pre_time_fps = millis();

      lcdPrintf(0,  0, white, "%d ms",  fps_time);
      lcdPrintf(0, 16, white, "%d fps", fps);


      lcdDrawFillRect(x, 32, 30, 30, red);
      lcdDrawFillRect(lcdGetWidth()-x, 62, 30, 30, green);
      lcdDrawFillRect(x + 30, 92, 30, 30, blue);

      if (touchGetTouchedCount() > 0)
      {
        touch_data_t data;

        for (int i=0; i<touchGetTouchedCount(); i++)
        {
          touchGetTouchedData(i, &data);
          lcdDrawFillRect(data.x-50, data.y-50, 100, 100, green);
        }
      }

      x += 2;

      x %= lcdGetWidth();
      y %= lcdGetHeight();

      lcdRequestDraw();
    }

    osThreadYield();
  }
}
