/**
 *******************************************************************************
 * @file    comm-spi-impl.cpp
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/22
 * @version 1.0
 *******************************************************************************
 */




/* ------- define --------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "comm-ospi.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_ospi.h"




/* ------- class prototypes-----------------------------------------------------------------------------------------*/



/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/

CommErr CommOspi::transmit(uint8_t *data, uint16_t len, OspiLineNum line) const {

    auto* hospi = static_cast<OSPI_HandleTypeDef*>(_handle);


    OSPI_RegularCmdTypeDef sCommand;

    sCommand.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG;
    sCommand.FlashId               = HAL_OSPI_FLASH_ID_1;

    sCommand.Instruction           = 0x9F;
    sCommand.InstructionMode       = HAL_OSPI_INSTRUCTION_NONE;
    sCommand.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS;
    sCommand.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE;

    sCommand.Address               = 0x0000;
    sCommand.AddressMode           = HAL_OSPI_ADDRESS_NONE;
    sCommand.AddressSize           = HAL_OSPI_ADDRESS_16_BITS;
    sCommand.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE;

    sCommand.AlternateBytes        = 0x00;
    sCommand.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE;
    sCommand.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_16_BITS;
    sCommand.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;

    //sCommand.DataMode              = static_cast<uint32_t>(line) << 24;
    sCommand.DataMode              = HAL_OSPI_DATA_1_LINE;
    sCommand.NbData                = len;
    sCommand.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE;

    sCommand.DummyCycles           = 0x0;
    sCommand.DQSMode               = HAL_OSPI_DQS_DISABLE;
    sCommand.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD;


    HAL_OSPI_Command(hospi, &sCommand, 0xFFFF);


    HAL_OSPI_Transmit_DMA(hospi, data);


    return CommErr::COMM_SUCCESS;
}

