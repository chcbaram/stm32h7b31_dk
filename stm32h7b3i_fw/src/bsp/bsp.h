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


bool bspInit(void);
bool bspDeInit(void);


void delay(uint32_t ms);
uint32_t millis(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_BSP_BSP_H_ */
