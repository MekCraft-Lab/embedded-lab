/**
 *******************************************************************************
 * @file    dev-bmi-cmd.h
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
 * @date    2025/11/26
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef H723VG_BMI08X_DEV_BMI_CMD_H
#define H723VG_BMI08X_DEV_BMI_CMD_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

enum class Bmi160Cmd : uint8_t {
    CHIP_ID      = 0x00,
    ERR          = 0x02,
    PMU_STATUS   = 0x03,
    DATA         = 0x04,
    SENSOR_TIME  = 0x18,
    STATUS       = 0x1B,
    INT_STATUS   = 0x1C,
    TEMPERATURE  = 0x1D,
    FIFO_LEN     = 0x22,
    FIFO_DATA    = 0x24,
    ACC_CONF     = 0x40,
    ACC_RANGE    = 0x41,
    GYR_CONF     = 0x42,
    GYR_RANGE    = 0x43,
    MAG_CONF     = 0x44,
    FIFO_DOWNS   = 0x45,
    FIFO_CONFIG  = 0x46,
    MAG_IF       = 0x4B,
    INT_EN       = 0x50,
    INT_OUT_CTRL = 0x53,
    INT_LATCH    = 0x54,
    INT_MAP      = 0x55,
    INT_DATA     = 0x58,
    INT_LOW_HIGH = 0x5A,
    INT_MOTION   = 0x5F,
    INT_TAP      = 0x63,
    INT_ORIENT   = 0x65,
    INT_FLAG     = 0x67,
    FOC_CONF     = 0x69,
    CONF         = 0x6A,
    IF_CONF      = 0x6B,
    PMU_TRIGGER  = 0x6C,
    SELF_TEST    = 0x6D,
    NV_CONF      = 0x70,
    OFFSET       = 0x71,
    STEP_CNT     = 0x78,
    STEP_CONF    = 0x7A,
    CMD          = 0x7E,
};


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*H723VG_BMI08X_DEV_BMI_CMD_H*/
