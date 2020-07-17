/*
 * mx25lm51245g.c
 *
 *  Created on: 2020. 7. 16.
 *      Author: Baram
 */




#include "qspi/mx25lm51245g.h"


#if defined(_USE_HW_QSPI) && HW_QSPI_DRIVER == MX25LM51245G

#define _FLASH_SIZE       0x4000000   /* 512   MBits => 64MBytes */
#define _BLOCK_SIZE       0x10000     /* 1024  blocks of 64KBytes */
#define _SECTOR_SIZE      0x4000      /* 16384 subsectors of 4kBytes */
#define _PAGE_SIZE        0x100       /* 65536 pages of 256 bytes */


#define SPI_CMD_RSTEN         0x66
#define SPI_CMD_RST           0x99
#define SPI_CMD_WREN          0x06
#define SPI_CMD_WRDI          0x04
#define SPI_CMD_RDSR          0x05
#define SPI_CMD_RDCR          0x15
#define SPI_CMD_WRCR2         0x72


#define OPI_CMD_RSTEN         0x6699
#define OPI_CMD_RST           0x9966
#define OPI_CMD_WREN          0x06F9
#define OPI_CMD_WRDI          0x04FB
#define OPI_CMD_RDID          0x9F60
#define OPI_CMD_8DTRD         0xEE11
#define OPI_CMD_8READ         0xEC13
#define OPI_CMD_PP            0x12ED
#define OPI_CMD_SE            0x21DE
#define OPI_CMD_BE            0xDC23
#define OPI_CMD_CE            0x609F
#define OPI_CMD_RDSR          0x05FA


#define DUMMY_CYCLES_READ            8U
#define DUMMY_CYCLES_READ_OCTAL      6U
#define DUMMY_CYCLES_READ_OCTAL_DTR  6U
#define DUMMY_CYCLES_REG_OCTAL       5U
#define DUMMY_CYCLES_REG_OCTAL_DTR   5U

#define CR2_DC                       0x07U        //    Dummy cycle
#define CR2_DC_20_CYCLES             0x00U        // 20 Dummy cycles
#define CR2_DC_18_CYCLES             0x01U        // 18 Dummy cycles
#define CR2_DC_16_CYCLES             0x02U        // 16 Dummy cycles
#define CR2_DC_14_CYCLES             0x03U        // 14 Dummy cycles
#define CR2_DC_12_CYCLES             0x04U        // 12 Dummy cycles
#define CR2_DC_10_CYCLES             0x05U        // 10 Dummy cycles
#define CR2_DC_8_CYCLES              0x06U        // 8 Dummy cycles
#define CR2_DC_6_CYCLES              0x07U        // 6 Dummy cycles



static bool init(void);
static bool getID(qspi_info_t *p_info);
static bool getInfo(qspi_info_t *p_info);
static bool reset(void);
static bool read(uint8_t *p_data, uint32_t addr, uint32_t length);
static bool write(uint8_t *p_data, uint32_t addr, uint32_t length);
static bool eraseBlock(uint32_t block_addr);
static bool eraseSector(uint32_t sector_addr);
static bool eraseChip(void);
static bool getStatus(void);
static bool enableMemoryMappedMode(void);
static bool disableMemoryMappedMode(void);

static uint32_t getFlashSize(void);
static uint32_t getSectorSize(void);
static uint32_t getBlockSize(void);
static uint32_t getPageSize(void);

static bool writeEnable(void);

static OSPI_HandleTypeDef hospi1;



bool mx25lm51245gInitDriver(qspi_driver_t *p_driver)
{
  p_driver->init = init;
  p_driver->getID = getID;
  p_driver->getInfo = getInfo;
  p_driver->reset = reset;
  p_driver->read = read;
  p_driver->write = write;
  p_driver->eraseBlock = eraseBlock;
  p_driver->eraseSector = eraseSector;
  p_driver->eraseChip = eraseChip;
  p_driver->getStatus = getStatus;
  p_driver->enableMemoryMappedMode = enableMemoryMappedMode;
  p_driver->disableMemoryMappedMode = disableMemoryMappedMode;
  p_driver->getFlashSize = getFlashSize;
  p_driver->getBlockSize = getBlockSize;
  p_driver->getSectorSize = getSectorSize;
  p_driver->getPageSize = getPageSize;

  return true;
}

bool init(void)
{
  bool ret = true;

  OSPIM_CfgTypeDef sOspiManagerCfg = {0};

  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 4;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
  hospi1.Init.DeviceSize = 26;
  hospi1.Init.ChipSelectHighTime = 2;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi1.Init.ClockPrescaler = 3;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.ClkChipSelectHighTime = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    return false;
  }

  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.DQSPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  sOspiManagerCfg.IOHighPort = HAL_OSPIM_IOPORT_1_HIGH;
  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  if (reset() != true)
  {
    return false;
  }

  return ret;
}


bool getID(qspi_info_t *p_info)
{
  bool ret = true;

  if (getInfo(p_info) != true)
  {
    return false;
  }

  if (p_info->device_id[0] != 0xC2 || p_info->device_id[1] != 0x85 || p_info->device_id[2] != 0x3A )
  {
    ret = false;
  }

  return ret;
}

bool getInfo(qspi_info_t *p_info)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef s_command = {0};


  p_info->FlashSize          = _FLASH_SIZE;
  p_info->EraseSectorSize    = _SECTOR_SIZE;
  p_info->EraseSectorsNumber = (_FLASH_SIZE/_SECTOR_SIZE);
  p_info->ProgPageSize       = _PAGE_SIZE;
  p_info->ProgPagesNumber    = (_FLASH_SIZE/_PAGE_SIZE);



  /* Initialize the read ID command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_RDID;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
  s_command.Address            = 0U;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_8_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 4;
  s_command.NbData             = 3U;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  memset(p_info->device_id, 0, sizeof(p_info->device_id));

  /* Reception of the data */
  if (HAL_OSPI_Receive(&hospi1, p_info->device_id, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }


  return ret;
}

bool reset(void)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef s_command = {0};
  OSPI_AutoPollingTypeDef sConfig;
  uint8_t reg;


  //-- SPI Reset Enable
  //
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.Instruction        = SPI_CMD_RSTEN;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0U;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  //-- SPI Reset Memoy
  //
  s_command.Instruction        = SPI_CMD_RST;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  //-- OPI Reset Enable
  //
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_ENABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_RSTEN;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  //-- OPI Reset Memoy
  //
  s_command.Instruction        = OPI_CMD_RST;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  delay(40);


  //-- SPI Write Enable
  //
  s_command.Instruction        = SPI_CMD_WREN;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  //-- Read Status
  //
  s_command.Instruction        = SPI_CMD_RDSR;
  s_command.DataMode           = HAL_OSPI_DATA_1_LINE;
  s_command.NbData             = 1;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  sConfig.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
  sConfig.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
  sConfig.Interval        = 0x10;
  sConfig.Match           = 0x02;
  sConfig.Mask            = 0x02;

  if (HAL_OSPI_AutoPolling(&hospi1, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  s_command.Instruction = SPI_CMD_WRCR2;
  s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  s_command.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
  s_command.DataMode    = HAL_OSPI_DATA_1_LINE;
  s_command.Address     = 0x300;
  reg = CR2_DC_6_CYCLES;

  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
  if (HAL_OSPI_Transmit(&hospi1, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
  delay(40);



  //-- SPI Write Enable
  //
  s_command.Instruction        = SPI_CMD_WREN;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;

  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  //-- Read Status
  //
  s_command.Instruction        = SPI_CMD_RDSR;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_1_LINE;
  s_command.NbData             = 1;
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  sConfig.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
  sConfig.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
  sConfig.Interval        = 0x10;
  sConfig.Match           = 0x02;
  sConfig.Mask            = 0x02;

  if (HAL_OSPI_AutoPolling(&hospi1, &sConfig, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  s_command.Instruction = SPI_CMD_WRCR2;
  s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
  s_command.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
  s_command.DataMode    = HAL_OSPI_DATA_1_LINE;
  s_command.Address     = 0;
  //reg = 0x2; // DTR OPI Enable
  reg = 0x1; // STR OPI Enable

  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }
  if (HAL_OSPI_Transmit(&hospi1, &reg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  delay(40);

  return ret;
}

bool read(uint8_t *p_data, uint32_t addr, uint32_t length)
{
  bool ret = true;

  OSPI_RegularCmdTypeDef s_command = {0};

  /* Initialize the read command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_8READ;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
  s_command.Address            = addr;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_8_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = DUMMY_CYCLES_READ_OCTAL;
  s_command.NbData             = length;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, 100) != HAL_OK)
  {
    return false;
  }

  /* Reception of the data */
  if (HAL_OSPI_Receive(&hospi1, p_data, 100) != HAL_OK)
  {
    return false;
  }

  return ret;
}

bool writePage(uint32_t addr, uint32_t data_addr)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef s_command = {0};

  if (writeEnable() != true)
  {
    return false;
  }

  /* Initialize the program command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_PP;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
  s_command.Address            = addr;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_8_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = 0U;
  s_command.NbData             = _PAGE_SIZE;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Configure the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Transmission of the data */
  if (HAL_OSPI_Transmit(&hospi1, (uint8_t *)data_addr, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  return ret;
}

bool write(uint8_t *p_data, uint32_t addr, uint32_t length)
{
  bool ret = true;
  uint32_t index;
  uint32_t write_length;
  uint32_t write_addr;
  uint32_t buf_mem[_PAGE_SIZE];
  uint8_t  *buf;
  uint32_t offset;


  index = 0;
  buf = (uint8_t *)buf_mem;
  offset = addr%_PAGE_SIZE;

  if (offset != 0 || length < _PAGE_SIZE)
  {
    write_addr = addr - offset;
    //memcpy(&buf[0], (void *)write_addr, _PAGE_SIZE);
    read(&buf[0], write_addr, _PAGE_SIZE);
    memcpy(&buf[offset], &p_data[0], constrain(_PAGE_SIZE-offset, 0, length));

    ret = writePage(write_addr, (uint32_t)buf);
    if (ret != true)
    {
      return false;
    }

    if (length < _PAGE_SIZE)
    {
      index += length;
    }
    else
    {
      index += (_PAGE_SIZE - offset);
    }
  }


  while(index < length)
  {
    write_length = constrain(length - index, 0, _PAGE_SIZE);

    ret = writePage(addr + index, (uint32_t)&p_data[index]);
    if (ret != true)
    {
      ret = false;
      break;
    }

    index += write_length;

    if ((length - index) > 0 && (length - index) < _PAGE_SIZE)
    {
      offset = length - index;
      write_addr = addr + index;
      //memcpy(&buf[0], (void *)write_addr, _PAGE_SIZE);
      read(&buf[0], write_addr, _PAGE_SIZE);
      memcpy(&buf[0], &p_data[index], offset);

      ret = writePage(write_addr, (uint32_t)buf);
      if (ret != true)
      {
        return false;
      }
      break;
    }
  }

  return ret;
}

bool eraseBlock(uint32_t block_addr)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef s_command = {0};

  /* Initialize the erase command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
  s_command.Address            = block_addr;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DummyCycles        = 0U;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  s_command.Instruction = OPI_CMD_BE;

  /* Send the command */
  if(HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  return ret;
}

bool eraseSector(uint32_t sector_addr)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef s_command = {0};


  if (writeEnable() != true)
  {
    return false;
  }

  /* Initialize the erase command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
  s_command.Address            = sector_addr;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DummyCycles        = 0U;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  s_command.Instruction = OPI_CMD_SE;

  /* Send the command */
  if(HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  return ret;
}

bool eraseChip(void)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef s_command = {0};

  if (writeEnable() != true)
  {
    return false;
  }


  /* Initialize the erase command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_CE;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DummyCycles        = 0U;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if(HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  return ret;
}

bool getStatus(void)
{
  bool ret = true;

  return ret;
}

bool enableMemoryMappedMode(void)
{
  bool ret = true;
  OSPI_RegularCmdTypeDef      s_command = {0};
  OSPI_MemoryMappedTypeDef s_mem_mapped_cfg = {0};

  /* Initialize the read command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_READ_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_8READ;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize        = HAL_OSPI_ADDRESS_32_BITS;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_8_LINES;
  s_command.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles        = DUMMY_CYCLES_READ_OCTAL;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the read command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Initialize the program command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_WRITE_CFG;
  s_command.Instruction        = OPI_CMD_PP;
  s_command.DummyCycles        = 0U;

  /* Send the write command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Configure the memory mapped mode */
  s_mem_mapped_cfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;

  if (HAL_OSPI_MemoryMapped(&hospi1, &s_mem_mapped_cfg) != HAL_OK)
  {
    return false;
  }

  return ret;
}

bool disableMemoryMappedMode(void)
{
  bool ret = true;

  return ret;
}

uint32_t getFlashSize(void)
{
  return _FLASH_SIZE;
}

uint32_t getSectorSize(void)
{
  return _SECTOR_SIZE;
}

uint32_t getBlockSize(void)
{
  return _BLOCK_SIZE;
}

uint32_t getPageSize(void)
{
  return _PAGE_SIZE;
}

bool writeEnable(void)
{
  OSPI_RegularCmdTypeDef     s_command = {0};
  OSPI_AutoPollingTypeDef s_config = {0};


  /* Initialize the write enable command */
  s_command.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  s_command.FlashId            = HAL_OSPI_FLASH_ID_1;
  s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
  s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
  s_command.InstructionSize    = HAL_OSPI_INSTRUCTION_16_BITS;
  s_command.Instruction        = OPI_CMD_WREN;
  s_command.AddressMode        = HAL_OSPI_ADDRESS_NONE;
  s_command.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
  s_command.DataMode           = HAL_OSPI_DATA_NONE;
  s_command.DummyCycles        = 0U;
  s_command.DQSMode            = HAL_OSPI_DQS_DISABLE;
  s_command.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;

  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  /* Configure automatic polling mode to wait for write enabling */
  s_command.Instruction    = OPI_CMD_RDSR;
  s_command.AddressMode    = HAL_OSPI_ADDRESS_8_LINES;
  s_command.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
  s_command.AddressSize    = HAL_OSPI_ADDRESS_32_BITS;
  s_command.Address        = 0U;
  s_command.DataMode       = HAL_OSPI_DATA_8_LINES;
  s_command.DataDtrMode    = HAL_OSPI_DATA_DTR_DISABLE;
  s_command.DummyCycles    = DUMMY_CYCLES_REG_OCTAL;
  s_command.NbData         = 1U;
  s_command.DQSMode        = HAL_OSPI_DQS_DISABLE;

  /* Send the command */
  if (HAL_OSPI_Command(&hospi1, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }

  s_config.Match           = 2U;
  s_config.Mask            = 2U;
  s_config.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
  s_config.Interval        = 0x10;
  s_config.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

  if (HAL_OSPI_AutoPolling(&hospi1, &s_config, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return false;
  }


  return true;
}



void HAL_OSPI_MspInit(OSPI_HandleTypeDef* ospiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(ospiHandle->Instance==OCTOSPI1)
  {
  /* USER CODE BEGIN OCTOSPI1_MspInit 0 */

  /* USER CODE END OCTOSPI1_MspInit 0 */
    /* OCTOSPI1 clock enable */
    __HAL_RCC_OCTOSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**OCTOSPI1 GPIO Configuration
    PG9     ------> OCTOSPIM_P1_IO6
    PD7     ------> OCTOSPIM_P1_IO7
    PG6     ------> OCTOSPIM_P1_NCS
    PF6     ------> OCTOSPIM_P1_IO3
    PF7     ------> OCTOSPIM_P1_IO2
    PF9     ------> OCTOSPIM_P1_IO1
    PD11     ------> OCTOSPIM_P1_IO0
    PC1     ------> OCTOSPIM_P1_IO4
    PH3     ------> OCTOSPIM_P1_IO5
    PC5     ------> OCTOSPIM_P1_DQS
    PB2     ------> OCTOSPIM_P1_CLK
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_OCTOSPIM_P1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN OCTOSPI1_MspInit 1 */

  /* USER CODE END OCTOSPI1_MspInit 1 */
  }
}

void HAL_OSPI_MspDeInit(OSPI_HandleTypeDef* ospiHandle)
{

  if(ospiHandle->Instance==OCTOSPI1)
  {
  /* USER CODE BEGIN OCTOSPI1_MspDeInit 0 */

  /* USER CODE END OCTOSPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_OCTOSPIM_CLK_DISABLE();
    __HAL_RCC_OSPI1_CLK_DISABLE();

    /**OCTOSPI1 GPIO Configuration
    PG9     ------> OCTOSPIM_P1_IO6
    PD7     ------> OCTOSPIM_P1_IO7
    PG6     ------> OCTOSPIM_P1_NCS
    PF6     ------> OCTOSPIM_P1_IO3
    PF7     ------> OCTOSPIM_P1_IO2
    PF9     ------> OCTOSPIM_P1_IO1
    PD11     ------> OCTOSPIM_P1_IO0
    PC1     ------> OCTOSPIM_P1_IO4
    PH3     ------> OCTOSPIM_P1_IO5
    PC5     ------> OCTOSPIM_P1_DQS
    PB2     ------> OCTOSPIM_P1_CLK
    */
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_9|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_7|GPIO_PIN_11);

    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_9);

    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2);

  /* USER CODE BEGIN OCTOSPI1_MspDeInit 1 */

  /* USER CODE END OCTOSPI1_MspDeInit 1 */
  }
}

#endif
