/*
 * ft5336.c
 *
 *  Created on: 2020. 7. 12.
 *      Author: Baram
 */





#include "touch/ft5336.h"

#ifdef _USE_HW_FT5336
#include "i2c.h"




#define FT5336_I2C_ADDR             0x38
#define FT5336_MAX_TOUCHES          5

#define FT5336_REG_DEVICE_MODE      0x00
#define FT5336_REG_GEST_ID          0x01
#define FT5336_REG_TD_STATUS        0x02
#define FT5336_REG_TOUCH1_XH        0x03

#define FT5336_ID_G_CHIPID          0x51


static uint8_t i2c_ch = _DEF_I2C1;
static bool is_init = false;

#ifdef _USE_HW_RTOS
static osMutexId mutex_id;
#endif


typedef struct
{
    uint8_t XH;
    uint8_t XL;
    uint8_t YH;
    uint8_t YL;
    uint8_t RESERVED[2];
} ft5336_touch_point_t;




static bool ft5336Init(void);
static uint8_t ft5336GetTouchedCount(void);
static bool ft5336GetTouchedData(uint8_t index, touch_data_t *p_data);
static bool ft5336ReadReg(uint16_t addr, uint8_t *p_data, uint32_t length);
static bool ft5336WriteReg(uint16_t addr, uint8_t *p_data, uint32_t length);




bool ft5336InitDriver(touch_driver_t *p_driver)
{
  p_driver->init = ft5336Init;
  p_driver->getTouchedCount = ft5336GetTouchedCount;
  p_driver->getTouchedData = ft5336GetTouchedData;

  return true;
}



bool ft5336Init(void)
{
  uint8_t data = 0;
  bool ret = false;


  if (i2cIsInit() != true) return false;

  if (i2cIsBegin(i2c_ch) != true)
  {
    i2cBegin(i2c_ch, 400);
  }

  if (ft5336ReadReg(FT5336_ID_G_CHIPID, &data, 1) == true)
  {
    ret = true;
  }
  else
  {
    ret = false;
  }

  data = 0;
  ft5336WriteReg(FT5336_REG_DEVICE_MODE, &data, 1);


  is_init = true;

#ifdef _USE_HW_RTOS
  osMutexDef(mutex_id);
  mutex_id = osMutexCreate (osMutex(mutex_id));
#endif

  return ret;
}

uint8_t ft5336GetGestureID(void)
{
  uint8_t ret = 0;
  uint8_t data;

#ifdef _USE_HW_RTOS
  osMutexWait(mutex_id, osWaitForever);
#endif

  if (ft5336ReadReg(FT5336_REG_GEST_ID, &data, 1) == true)
  {
    ret = data;
  }

#ifdef _USE_HW_RTOS
  osMutexRelease(mutex_id);
#endif

  return ret;
}

uint8_t ft5336GetTouchedCount(void)
{
  uint8_t ret = 0;
  uint8_t data;

#ifdef _USE_HW_RTOS
  osMutexWait(mutex_id, osWaitForever);
#endif

  if (ft5336ReadReg(FT5336_REG_TD_STATUS, &data, 1) == true)
  {
    ret = data & 0x0F;

    if (ret > FT5336_MAX_TOUCHES)
    {
      ret = 0;
    }
  }

#ifdef _USE_HW_RTOS
  osMutexRelease(mutex_id);
#endif
  return ret;
}

bool ft5336GetTouchedData(uint8_t index, touch_data_t *p_data)
{
  bool ret = false;
  ft5336_touch_point_t data;

  if (index >= FT5336_MAX_TOUCHES)
  {
    return false;
  }

#ifdef _USE_HW_RTOS
  osMutexWait(mutex_id, osWaitForever);
#endif

  if (ft5336ReadReg(FT5336_REG_TOUCH1_XH + index*6, (uint8_t *)&data, 6) == true)
  {
    p_data->event = data.XH >> 6;
    p_data->id    = data.YH >> 4;

    p_data->y  = data.XL;
    p_data->y |= (data.XH & 0x0F) << 8;
    p_data->x  = data.YL;
    p_data->x |= (data.YH & 0x0F) << 8;

    ret = true;
  }

#ifdef _USE_HW_RTOS
  osMutexRelease(mutex_id);
#endif
  return ret;
}


bool ft5336ReadReg(uint16_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret;

  ret = i2cReadBytes(i2c_ch, FT5336_I2C_ADDR, addr, p_data, length, 10);

  return ret;
}

bool ft5336WriteReg(uint16_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret;

  ret = i2cWriteBytes(i2c_ch, FT5336_I2C_ADDR, addr, p_data, length, 10);

  return ret;
}





#endif //#ifdef _USE_HW_FT5336
