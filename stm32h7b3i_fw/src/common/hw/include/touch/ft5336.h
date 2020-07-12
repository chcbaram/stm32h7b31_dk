/*
 * ft5336.h
 *
 *  Created on: 2020. 7. 12.
 *      Author: Baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_FT5336_H_
#define SRC_COMMON_HW_INCLUDE_FT5336_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"

#ifdef _USE_HW_FT5336


#include "touch.h"


bool ft5336InitDriver(touch_driver_t *p_driver);




#ifdef __cplusplus
}
#endif


#endif


#endif /* SRC_COMMON_HW_INCLUDE_FT5336_H_ */
