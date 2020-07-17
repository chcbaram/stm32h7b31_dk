/*
 * mx25lm51245g.h
 *
 *  Created on: 2020. 7. 16.
 *      Author: Baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_QSPI_MX25LM51245G_H_
#define SRC_COMMON_HW_INCLUDE_QSPI_MX25LM51245G_H_



#ifdef __cplusplus
 extern "C" {
#endif


#include "hw_def.h"


#if defined(_USE_HW_QSPI) && HW_QSPI_DRIVER == MX25LM51245G

#include "qspi.h"



bool mx25lm51245gInitDriver(qspi_driver_t *p_driver);


#endif

#ifdef __cplusplus
}
#endif


#endif /* SRC_COMMON_HW_INCLUDE_QSPI_MX25LM51245G_H_ */
