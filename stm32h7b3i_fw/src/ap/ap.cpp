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
}

void apMain(void)
{
  uint32_t pre_time;

  ledOn(_DEF_LED1);
  ledOff(_DEF_LED2);

  pre_time = millis();
  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();

      ledToggle(_DEF_LED1);
      ledToggle(_DEF_LED2);
    }

    if (uartAvailable(_DEF_UART1) > 0)
    {
      uartPrintf(_DEF_UART1, "rx 0x%02X\n", uartRead(_DEF_UART1));
    }
  }

}
