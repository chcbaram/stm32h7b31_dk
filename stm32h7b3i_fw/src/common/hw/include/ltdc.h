/*
 * ltdc.h
 *
 *  Created on: 2020. 7. 18.
 *      Author: Baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_LTDC_H_
#define SRC_COMMON_HW_INCLUDE_LTDC_H_


#ifdef __cplusplus
 extern "C" {
#endif


#include "hw_def.h"

#ifdef _USE_HW_LTDC

enum class_color {
 white     = 0xFFFF,
 gray      = 0x8410,
 darkgray  = 0xAD55,
 black     = 0x0000,
 purple    = 0x8010,
 pink      = 0xFE19,
 red       = 0xF800,
 orange    = 0xFD20,
 brown     = 0xA145,
 beige     = 0xF7BB,
 yellow    = 0xFFE0,
 lightgreen= 0x9772,
 green     = 0x0400,
 darkblue  = 0x0011,
 blue      = 0x001F,
 lightblue = 0xAEDC,
};



bool ltdcInit(void);
bool ltdcDrawAvailable(void);
void ltdcRequestDraw(void);
void ltdcSetAlpha(uint16_t LayerIndex, uint32_t value);
uint16_t *ltdcGetFrameBuffer(void);
uint16_t *ltdcGetCurrentFrameBuffer(void);
int32_t  ltdcWidth(void);
int32_t  ltdcHeight(void);
uint32_t ltdcGetBufferAddr(uint8_t index);
bool ltdcLayerInit(uint16_t LayerIndex, uint32_t Address);
void ltdcSetDoubleBuffer(bool enable);
bool ltdcGetDoubleBuffer(void);


#endif

#ifdef __cplusplus
}
#endif


#endif /* SRC_COMMON_HW_INCLUDE_LTDC_H_ */
