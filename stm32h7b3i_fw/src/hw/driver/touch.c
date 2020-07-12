/*
 * touch.c
 *
 *  Created on: 2020. 7. 12.
 *      Author: Baram
 */



#include "touch.h"
#include "touch/ft5336.h"

#include "cmdif.h"



#ifdef _USE_HW_CMDIF
static void touchCmdif(void);
#endif



static bool is_init = false;
static touch_driver_t touch;

bool touchInit(void)
{
  is_init = ft5336InitDriver(&touch);


  if (is_init)
  {
    if (touch.init() != true)
    {
      is_init = false;
    }
  }

#ifdef _USE_HW_CMDIF
  cmdifAdd("touch", touchCmdif);
#endif
  return is_init;
}

uint8_t touchGetTouchedCount(void)
{
  if (is_init)
  {
    return touch.getTouchedCount();
  }
  else
  {
    return 0;
  }
}

bool touchGetTouchedData(uint8_t index, touch_data_t *p_data)
{
  if (is_init)
  {
    return touch.getTouchedData(index, p_data);
  }
  else
  {
    return false;
  }
}




#ifdef _USE_HW_CMDIF
void touchCmdif(void)
{
  bool ret = true;


  if (cmdifGetParamCnt() == 1  && cmdifHasString("info", 0) == true)
  {
    uint8_t touch_cnt;
    touch_data_t touch_data;

    while(cmdifRxAvailable() == 0)
    {
      touch_cnt = touchGetTouchedCount();
      if ( touch_cnt> 0)
      {
        cmdifPrintf("Touch : %d, ", touch_cnt);

        for (int i=0; i<touch_cnt; i++)
        {
          touchGetTouchedData(i, &touch_data);
          cmdifPrintf("[%d %d, x=%03d y=%03d] ", touch_data.event, touch_data.id, touch_data.x, touch_data.y);
        }
        cmdifPrintf("\n");
      }
      delay(10);
    }
  }
  else
  {
    ret = false;
  }


  if (ret == false)
  {
    cmdifPrintf( "ft5406 info \n");
  }
}
#endif
