/*
 * ap.cpp
 *
 *  Created on: 2020. 7. 10.
 *      Author: Baram
 */




#include "ap.h"



__attribute__((section(".ext_tag"))) const char board_name[] = { 0x01, 0x02, 0x03, 0x04, 0x06 };




void apInit(void)
{
  hwInit();


  cmdifOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;

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
    }

    osThreadYield();
  }
}
