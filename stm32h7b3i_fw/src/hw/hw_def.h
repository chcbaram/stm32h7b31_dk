/*
 * hw_def.h
 *
 *  Created on: 2020. 7. 10.
 *      Author: Baram
 */

#ifndef SRC_HW_HW_DEF_H_
#define SRC_HW_HW_DEF_H_


#include "def.h"
#include "bsp.h"



#define _HW_DEF_RTOS_MEM_SIZE(x)              ((x)/4)

#define _HW_DEF_RTOS_THREAD_PRI_MAIN          osPriorityNormal



#define _HW_DEF_RTOS_THREAD_MEM_MAIN          _HW_DEF_RTOS_MEM_SIZE( 6*1024)



#define _USE_HW_RTOS
#define _USE_HW_FT5336
#define _USE_HW_TOUCH
#define _USE_HW_FLASH


#define _USE_HW_LED
#define      HW_LED_MAX_CH          2

#define _USE_HW_UART
#define      HW_UART_MAX_CH         1

#define _USE_HW_SWTIMER
#define      HW_SWTIMER_MAX_CH      8

#define _USE_HW_CMDIF
#define      HW_CMDIF_LIST_MAX              32
#define      HW_CMDIF_CMD_STR_MAX           16
#define      HW_CMDIF_CMD_BUF_LENGTH        128

#define _USE_HW_BUTTON
#define      HW_BUTTON_MAX_CH       1

#define _USE_HW_SDRAM
#define      HW_SDRAM_MEM_ADDR      0xD0000000
#define      HW_SDRAM_MEM_SIZE      (16*1024*1024)

#define _USE_HW_GPIO
#define      HW_GPIO_MAX_CH         1

#define _USE_HW_I2C
#define      HW_I2C_MAX_CH          1


#define _USE_HW_QSPI
#define      HW_QSPI_DRIVER         MX25LM51245G
#define      HW_QSPI_BASE_ADDR      OCTOSPI1_BASE


#define _USE_TIMER_MICROS           TIM2
#define _USE_TIMER_SYSTICK          TIM7


#endif /* SRC_HW_HW_DEF_H_ */
