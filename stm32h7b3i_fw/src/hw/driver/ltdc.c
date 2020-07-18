/*
 * ltdc.c
 *
 *  Created on: 2020. 7. 18.
 *      Author: Baram
 */




#include "ltdc.h"
#include "sdram.h"
#include "gpio.h"

#ifdef _USE_HW_LTDC


#define FRAME_BUF_ADDR        SDRAM_ADDR_IMAGE
#define FRAME_OSD_ADDR        (SDRAM_ADDR_IMAGE + (1024+512)*1024)


#define LCD_WIDTH             ((uint16_t)480)   /* LCD PIXEL WIDTH            */
#define LCD_HEIGHT            ((uint16_t)272)   /* LCD PIXEL HEIGHT           */


#define LCD_HSYNC             ((uint16_t)41)    /* Horizontal synchronization */
#define LCD_HBP               ((uint16_t)13)    /* Horizontal back porch      */
#define LCD_HFP               ((uint16_t)32)    /* Horizontal front porch     */


#define LCD_VSYNC             ((uint16_t)10)    /* Vertical synchronization   */
#define LCD_VBP               ((uint16_t)2)     /* Vertical back porch        */
#define LCD_VFP               ((uint16_t)2)     /* Vertical front porch       */







void ltdcSetFrameBuffer(uint16_t* addr);



static LTDC_HandleTypeDef hltdc;
static volatile bool ltdc_request_draw = false;

static volatile uint16_t lcd_int_active_line;
static volatile uint16_t lcd_int_porch_line;


static volatile uint32_t  frame_index = 0;
static uint16_t *frame_buffer[2] =
    {
      (uint16_t *)(FRAME_BUF_ADDR + LCD_WIDTH*LCD_HEIGHT*2*0),
      (uint16_t *)(FRAME_BUF_ADDR + LCD_WIDTH*LCD_HEIGHT*2*1),
    };

uint16_t *ltdc_draw_buffer;
uint16_t *ltdc_osd_draw_buffer = (uint16_t *)FRAME_OSD_ADDR;

static volatile bool is_double_buffer = true;



bool ltdcInit(void)
{
  bool ret = true;

  /* LTDC Initialization -------------------------------------------------------*/
  /* DeInit */
  HAL_LTDC_DeInit(&hltdc);

  /* Polarity configuration */
  /* Initialize the horizontal synchronization polarity as active low */
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  /* Initialize the vertical synchronization polarity as active low */
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  /* Initialize the data enable polarity as active low */
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  /* Initialize the pixel clock polarity as input pixel clock */
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  /* Timing configuration */
  /* The LCD AMPIRE 640x480 is selected */
  /* Timing configuration */
  hltdc.Init.HorizontalSync = (LCD_HSYNC - 1);
  hltdc.Init.VerticalSync = (LCD_VSYNC - 1);
  hltdc.Init.AccumulatedHBP = (LCD_HSYNC + LCD_HBP - 1);
  hltdc.Init.AccumulatedVBP = (LCD_VSYNC + LCD_VBP - 1);
  hltdc.Init.AccumulatedActiveH = (LCD_HEIGHT + LCD_VSYNC + LCD_VBP - 1);
  hltdc.Init.AccumulatedActiveW = (LCD_WIDTH + LCD_HSYNC + LCD_HBP - 1);
  hltdc.Init.TotalHeigh = (LCD_HEIGHT + LCD_VSYNC + LCD_VBP + LCD_VFP - 1);
  hltdc.Init.TotalWidth = (LCD_WIDTH + LCD_HSYNC + LCD_HBP + LCD_HFP - 1);


  /* Configure R,G,B component values for LCD background color */
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;

  hltdc.Instance = LTDC;


  /* Configure the LTDC */
  if(HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    /* Initialization Error */
    ret = false;
  }


  uint16_t *p_data;

  p_data = (uint16_t *)frame_buffer[0];
  for (int i=0; i<LCD_WIDTH*LCD_HEIGHT; i++)
  {
    p_data[i] = black;
  }
  p_data = (uint16_t *)frame_buffer[1];
  for (int i=0; i<LCD_WIDTH*LCD_HEIGHT; i++)
  {
    p_data[i] = black;
  }


  ltdcLayerInit(LTDC_LAYER_1, (uint32_t)frame_buffer[frame_index]);
  ltdcLayerInit(LTDC_LAYER_2, FRAME_OSD_ADDR);
  ltdcSetAlpha(LTDC_LAYER_2, 0);


  if (is_double_buffer == true)
  {
    ltdc_draw_buffer = frame_buffer[frame_index ^ 1];
  }
  else
  {
    ltdc_draw_buffer = frame_buffer[frame_index];
  }

  lcd_int_active_line = (LTDC->BPCR & 0x7FF) - 1;
  lcd_int_porch_line  = (LTDC->AWCR & 0x7FF) - 1;

  HAL_LTDC_ProgramLineEvent(&hltdc, lcd_int_active_line);
  __HAL_LTDC_ENABLE_IT(&hltdc, LTDC_IT_LI | LTDC_IT_FU);

  NVIC_SetPriority(LTDC_IRQn, 5);
  NVIC_EnableIRQ(LTDC_IRQn);


  gpioPinWrite(_PIN_GPIO_LCD_BK_EN, _DEF_HIGH);

  ltdcRequestDraw();

  return ret;
}



void ltdcSetAlpha(uint16_t LayerIndex, uint32_t value)
{
HAL_LTDC_SetAlpha(&hltdc, value, LayerIndex);
}


bool ltdcLayerInit(uint16_t LayerIndex, uint32_t Address)
{
  LTDC_LayerCfgTypeDef      pLayerCfg;
  bool ret = true;


  /* Layer1 Configuration ------------------------------------------------------*/

  /* Windowing configuration */
  /* In this case all the active display area is used to display a picture then :
     Horizontal start = horizontal synchronization + Horizontal back porch = 43
     Vertical start   = vertical synchronization + vertical back porch     = 12
     Horizontal stop = Horizontal start + window width -1 = 43 + 480 -1
     Vertical stop   = Vertical start + window height -1  = 12 + 272 -1      */
  if (LayerIndex == LTDC_LAYER_1)
  {
    pLayerCfg.WindowX0 = 0;
    pLayerCfg.WindowX1 = LCD_WIDTH;
    pLayerCfg.WindowY0 = 0;
    pLayerCfg.WindowY1 = LCD_HEIGHT;
  }
  else
  {
    pLayerCfg.WindowX0 = (LCD_WIDTH-200)/2;
    pLayerCfg.WindowX1 = pLayerCfg.WindowX0 + 200;
    pLayerCfg.WindowY0 = (LCD_HEIGHT-200)/2;
    pLayerCfg.WindowY1 = pLayerCfg.WindowY0 + 200;
  }


  /* Pixel Format configuration*/
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;

  /* Start Address configuration : frame buffer is located at FLASH memory */
  pLayerCfg.FBStartAdress = Address;

  /* Alpha constant (255 == totally opaque) */
  pLayerCfg.Alpha = 255;

  /* Default Color configuration (configure A,R,G,B component values) : no background color */
  pLayerCfg.Alpha0 = 0; /* fully transparent */
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;

  /* Configure blending factors */
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;

  /* Configure the number of lines and number of pixels per line */
  pLayerCfg.ImageWidth  = LCD_WIDTH;
  pLayerCfg.ImageHeight = LCD_HEIGHT;


  /* Configure the Layer*/
  if(HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, LayerIndex) != HAL_OK)
  {
    /* Initialization Error */
    ret = false;
  }


  return ret;
}


void ltdcSetFrameBuffer(uint16_t* addr)
{
  LTDC_Layer1->CFBAR = (uint32_t)addr;

  /* Reload immediate */
  LTDC->SRCR = (uint32_t)LTDC_SRCR_IMR;
}


int32_t ltdcWidth(void)
{
  return LCD_WIDTH;
}

int32_t ltdcHeight(void)
{
  return LCD_HEIGHT;
}

uint32_t ltdcGetBufferAddr(uint8_t index)
{
  return  (uint32_t)frame_buffer[frame_index];
}

bool ltdcDrawAvailable(void)
{
  return !ltdc_request_draw;
}

void ltdcRequestDraw(void)
{
  ltdc_request_draw = true;
}

void ltdcSetDoubleBuffer(bool enable)
{
  is_double_buffer = enable;

  if (enable == true)
  {
    ltdc_draw_buffer = frame_buffer[frame_index^1];
  }
  else
  {
    ltdc_draw_buffer = frame_buffer[frame_index];
  }
}

bool ltdcGetDoubleBuffer(void)
{
  return is_double_buffer;
}

uint16_t *ltdcGetFrameBuffer(void)
{
  return  ltdc_draw_buffer;
}

uint16_t *ltdcGetCurrentFrameBuffer(void)
{
  return  frame_buffer[frame_index];
}


void ltdcSwapFrameBuffer(void)
{
  if (ltdc_request_draw == true)
  {
    ltdc_request_draw = false;

    frame_index ^= 1;

    ltdcSetFrameBuffer(frame_buffer[frame_index]);

    if (is_double_buffer == true)
    {
      ltdc_draw_buffer = frame_buffer[frame_index ^ 1];
    }
    else
    {
      ltdc_draw_buffer = frame_buffer[frame_index];
    }
  }
}









void LTDC_IRQHandler(void)
{
  HAL_LTDC_IRQHandler(&hltdc);
}


void HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef* hltdc)
{
  if (LTDC->LIPCR == lcd_int_active_line)
  {
    ltdcSwapFrameBuffer();
    HAL_LTDC_ProgramLineEvent(hltdc, lcd_int_active_line);
  }
  else
  {
    HAL_LTDC_ProgramLineEvent(hltdc, lcd_int_active_line);
  }
}



/** @defgroup HAL_MSP_Private_Functions
  * @{
  */

/**
  * @brief LTDC MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
 GPIO_InitTypeDef GPIO_InitStruct;

 /* Enable the LTDC and DMA2D clocks */
 __HAL_RCC_LTDC_CLK_ENABLE();
 __HAL_RCC_DMA2D_CLK_ENABLE();

 /** @brief Toggle Sw reset of LTDC IP */
 __HAL_RCC_LTDC_FORCE_RESET();
 __HAL_RCC_LTDC_RELEASE_RESET();

 /** @brief Toggle Sw reset of DMA2D IP */
 __HAL_RCC_DMA2D_FORCE_RESET();
 __HAL_RCC_DMA2D_RELEASE_RESET();


 __HAL_RCC_GPIOK_CLK_ENABLE();
 __HAL_RCC_GPIOJ_CLK_ENABLE();
 __HAL_RCC_GPIOI_CLK_ENABLE();
 /**LTDC GPIO Configuration
 PK5     ------> LTDC_B6
 PK6     ------> LTDC_B7
 PK3     ------> LTDC_B4
 PJ15     ------> LTDC_B3
 PK4     ------> LTDC_B5
 PJ14     ------> LTDC_B2
 PK7     ------> LTDC_DE
 PJ13     ------> LTDC_B1
 PJ12     ------> LTDC_B0
 PI12     ------> LTDC_HSYNC
 PI14     ------> LTDC_CLK
 PI13     ------> LTDC_VSYNC
 PK2     ------> LTDC_G7
 PK1     ------> LTDC_G6
 PJ11     ------> LTDC_G4
 PK0     ------> LTDC_G5
 PJ10     ------> LTDC_G3
 PJ9     ------> LTDC_G2
 PJ8     ------> LTDC_G1
 PJ6     ------> LTDC_R7
 PJ7     ------> LTDC_G0
 PI15     ------> LTDC_R0
 PJ0     ------> LTDC_R1
 PJ5     ------> LTDC_R6
 PJ1     ------> LTDC_R2
 PJ4     ------> LTDC_R5
 PJ2     ------> LTDC_R3
 PJ3     ------> LTDC_R4
 */
 GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_3|GPIO_PIN_4
                       |GPIO_PIN_7|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0;
 GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
 GPIO_InitStruct.Pull = GPIO_NOPULL;
 GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
 GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
 HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

 GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_12
                       |GPIO_PIN_11|GPIO_PIN_10|GPIO_PIN_9|GPIO_PIN_8
                       |GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_0|GPIO_PIN_5
                       |GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_2|GPIO_PIN_3;
 GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
 GPIO_InitStruct.Pull = GPIO_NOPULL;
 GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
 GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
 HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

 GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_13|GPIO_PIN_15;
 GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
 GPIO_InitStruct.Pull = GPIO_NOPULL;
 GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
 GPIO_InitStruct.Alternate = GPIO_AF14_LTDC;
 HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

}

/**
  * @brief LTDC MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  * @param hltdc: LTDC handle pointer
  * @retval None
  */
void HAL_LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc)
{

  /*##-1- Reset peripherals ##################################################*/
  /* Enable LTDC reset state */
  __HAL_RCC_LTDC_FORCE_RESET();

  /* Release LTDC from reset state */
  __HAL_RCC_LTDC_RELEASE_RESET();
}



#endif
