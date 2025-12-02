/**
 *******************************************************************************
 * @file    w25qxx-comm-adpter.cpp
 * @brief   The communication adapter layer of w25qxx driver.
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
 * @date    2025/8/24
 * @version 1.0
 *******************************************************************************
 */


/* ------- define --------------------------------------------------------------------------------------------------*/


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_ospi.h"
#include "w25qxx.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/
static W25QxxErr cmdConfig(const std::vector<W25QxxMsgElement>& msg, OSPI_RegularCmdTypeDef* cmd, uint8_t** pData);

/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/


/* ------- function implement ----------------------------------------------------------------------------------------*/


W25QxxErr W25QxxSpiComm::transmit(const std::vector<W25QxxMsgElement>& msg) const {

    uint8_t* pData                  = nullptr;
    W25QxxErr ret                   = W25QxxErr::NONE;
    OSPI_RegularCmdTypeDef sCommand = {};

    ret                             = cmdConfig(msg, &sCommand, &pData);

    if (ret != W25QxxErr::SUCCESS) {
        return ret;
    }



    if (sCommand.DataMode == 0) {
        HAL_OSPI_Command_IT(static_cast<OSPI_HandleTypeDef*>(_commHandle), &sCommand);
    } else {
        HAL_OSPI_Command(static_cast<OSPI_HandleTypeDef*>(_commHandle), &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
    }
    HAL_OSPI_Transmit_DMA(static_cast<OSPI_HandleTypeDef*>(_commHandle), pData);
    return W25QxxErr::SUCCESS;
}

W25QxxErr W25QxxSpiComm::receive(const std::vector<W25QxxMsgElement>& msg) const {

    uint8_t* pData                  = nullptr;
    W25QxxErr ret                   = W25QxxErr::NONE;
    OSPI_RegularCmdTypeDef sCommand = {};

    ret                             = cmdConfig(msg, &sCommand, &pData);

    if (ret != W25QxxErr::SUCCESS) {
        return ret;
    }
    HAL_OSPI_Command(static_cast<OSPI_HandleTypeDef*>(_commHandle), &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

    HAL_OSPI_Receive_DMA(static_cast<OSPI_HandleTypeDef*>(_commHandle), pData);

    return W25QxxErr::SUCCESS;
}

W25QxxErr W25QxxSpiComm::autoPolling(std::vector<W25QxxMsgElement> &msgs, uint32_t target, uint32_t mask,
    uint8_t interval) {


    uint8_t* pData                  = nullptr;
    W25QxxErr ret                   = W25QxxErr::NONE;
    OSPI_RegularCmdTypeDef sCommand = {};

    ret                             = cmdConfig(msgs, &sCommand, &pData);

    if (ret != W25QxxErr::SUCCESS) {
        return ret;
    }

    uint32_t state = HAL_OSPI_GetState(static_cast<OSPI_HandleTypeDef*>(_commHandle));
    if (state == HAL_OSPI_STATE_BUSY_TX || state == HAL_OSPI_STATE_BUSY_CMD) {
        ret = W25QxxErr::BUSY;
        return ret;
    } else if (state != HAL_OSPI_STATE_READY) {
        ret = W25QxxErr::INVALID;
        return ret;
    }

    if (HAL_OSPI_Command(static_cast<OSPI_HandleTypeDef*>(_commHandle), &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        ret = W25QxxErr::INVALID;
        return ret;
    }

    OSPI_AutoPollingTypeDef aConfig = {};
    aConfig.Match = target;
    aConfig.Mask = mask;
    aConfig.Interval = interval;
    aConfig.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;
    aConfig.MatchMode = HAL_OSPI_MATCH_MODE_AND;

    if (HAL_OSPI_AutoPolling_IT(static_cast<OSPI_HandleTypeDef*>(_commHandle), &aConfig) != HAL_OK) {
        ret = W25QxxErr::INVALID;
        return ret;
    }


    return ret;
}

static W25QxxErr cmdConfig(const std::vector<W25QxxMsgElement>& msg, OSPI_RegularCmdTypeDef* cmd, uint8_t** pData) {

    cmd->OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd->FlashId       = HAL_OSPI_FLASH_ID_1;
    cmd->DQSMode       = HAL_OSPI_DQS_DISABLE;
    cmd->SIOOMode      = HAL_OSPI_SIOO_INST_EVERY_CMD;


    for (auto& eachMsg : msg) {
        switch (eachMsg.getType()) {

            case MsgTypeEnum::INSTRUCTION: {
                cmd->Instruction        = eachMsg.getByte();
                cmd->InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
                cmd->InstructionSize    = (eachMsg.getNum() - 1) << 4;
                cmd->InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;

            } break;

            case MsgTypeEnum::ADDRESS: {
                cmd->Address        = eachMsg.getByte();
                cmd->AddressMode    = eachMsg.getLines() << 8;
                cmd->AddressSize    = (eachMsg.getNum() - 1) << 12;
                cmd->AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
            } break;

            case MsgTypeEnum::ALTERNATE_BYTE: {
                cmd->AlternateBytes        = eachMsg.getByte();
                cmd->AlternateBytesMode    = eachMsg.getLines() << 16;
                cmd->AlternateBytesSize    = (eachMsg.getNum() - 1) << 20;
                cmd->AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
            } break;

            case MsgTypeEnum::DUMMY_CYCLE: {
                cmd->DummyCycles = eachMsg.getNum();
            } break;

            case MsgTypeEnum::DATA: {
                cmd->DataMode    = eachMsg.getLines() << 24;
                cmd->NbData      = eachMsg.getNum();
                cmd->DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
                *pData           = eachMsg.getPData();
            } break;
            default:
                return W25QxxErr::INVALID;
        }
    }

    return W25QxxErr::SUCCESS;
}
