/*
 * bsp.h
 *
 *  Created on: 2020. 7. 10.
 *      Author: Baram
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "def.h"

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"


#define logPrintf(...)    printf(__VA_ARGS__)


bool bspInit(void);
bool bspDeInit(void);


void Error_Handler(void);
void delay(uint32_t ms);
void delayUs(uint32_t us);
uint32_t millis(void);
uint32_t micros(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_BSP_BSP_H_ */
