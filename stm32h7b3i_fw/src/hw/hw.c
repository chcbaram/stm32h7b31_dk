/*
 * hw.c
 *
 *  Created on: 2020. 7. 10.
 *      Author: Baram
 */




#include "hw.h"





void hwInit(void)
{
  bspInit();

  cmdifInit();
  swtimerInit();

  ledInit();
  uartInit();
  uartOpen(_DEF_UART1, 57600);

  buttonInit();

}
