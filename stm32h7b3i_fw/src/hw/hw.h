/*
 * hw.h
 *
 *  Created on: 2020. 7. 10.
 *      Author: Baram
 */

#ifndef SRC_HW_HW_H_
#define SRC_HW_HW_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#include "led.h"
#include "uart.h"
#include "cmdif.h"
#include "swtimer.h"
#include "button.h"
#include "sdram.h"
#include "gpio.h"
#include "i2c.h"
#include "touch.h"
#include "flash.h"
#include "qspi.h"
#include "sd.h"
#include "ltdc.h"
#include "lcd.h"

void hwInit(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_HW_HW_H_ */
