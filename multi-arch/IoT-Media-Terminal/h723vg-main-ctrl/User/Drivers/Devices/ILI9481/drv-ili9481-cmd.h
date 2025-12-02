/**
 *******************************************************************************
 * @file    drv-ili9841-cmd.h
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
 * @date    2025/11/15
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef VGT6_FMC_LCD_DRV_ILI9841_CMD_H
#define VGT6_FMC_LCD_DRV_ILI9841_CMD_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

enum class ILI9481_CMD {
    NOP        = 0x00,
    SOFT_RESET = 0x01,
    ENTER_SLEEP = 0x10,
    EXIT_SLEEP = 0x11,
    ENTER_PARTIAL = 0x12,
    ENTER_NORMAL = 0x13,
    EXIT_INVERT = 0x20,
    ENTER_INVERT = 0x21,
    SET_GAMMA_CURVE = 0x26,
    SET_DISPLAY_OFF = 0x28,
    SET_DISPLAY_ON = 0x29,
    SET_COL_ADDR = 0x2A,
    SET_PAGE_ADDR = 0x2B,
    WRITE_MEM_START = 0x2C,
    WRITE_LUT = 0x2D,
    SET_PARTIAL_AREA = 0x30,
    SET_SCROLL_AREA = 0x33,
    SET_TEAR_OFF = 0x34,
    SET_TEAR_ON = 0x35,
    SET_ADDR_MODE = 0x36,
    SET_SCROLL_START = 0x37,
    EXIT_IDLE = 0x38,
    ENTER_IDLE = 0x39,
    SET_PIXEL_FORMAT = 0x3A,
    WRITE_MEM_CONTINUE = 0x3C,
    SET_TEAR_SCANLINE = 0x44,
    PANEL_DRIVING_SETTING = 0xC0,
    FRAME_RATE_AND_INVERSION_CONTROL = 0xC5,
    GAMMA_SETTING = 0xC8,
    POWER_SETTING = 0xD0,
    VCOM_CONTROL = 0xD1,
    POWER_SETTING_FOR_NORMAL_MODE = 0xD2,

};


/*-------- 3. interface ---------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*VGT6_FMC_LCD_DRV_ILI9841_CMD_H*/
