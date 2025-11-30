/**
 *******************************************************************************
 * @file    driv-ili9481-adaptor.h
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
 * @date    2025/11/30
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef VGT6_FMC_LCD_DRIV_ILI_9481_ADAPTOR_H
#define VGT6_FMC_LCD_DRIV_ILI_9481_ADAPTOR_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../../Devices/ILI9481/drv-ili9481.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define __IO volatile /*!< Defines 'read / write' permissions */




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 初始化LCD
 */
void lcdInit(void);

#define __FILL_RECT(x, y, x_end, y_end, color)                                                                         \
    do {                                                                                                               \
        __WRITE_REG(0x2A);                                                                                             \
        __WRITE_DATA(x >> 8);                                                                                          \
        __WRITE_DATA(x & 0xFF);                                                                                        \
        __WRITE_DATA(x_end >> 8);                                                                                      \
        __WRITE_DATA(x_end & 0xFF);                                                                                    \
                                                                                                                       \
        __WRITE_REG(0x2B);                                                                                             \
        __WRITE_DATA(y >> 8);                                                                                          \
        __WRITE_DATA(y & 0xFF);                                                                                        \
        __WRITE_DATA(y_end >> 8);                                                                                      \
        __WRITE_DATA(y_end & 0xFF);                                                                                    \
                                                                                                                       \
        __WRITE_REG(0x2C);                                                                                             \
                                                                                                                       \
        uint32_t total = (x_end - x + 1) * (y_end - y + 1);                                                            \
                                                                                                                       \
                                                                                                                       \
        uint32_t cnt   = total;                                                                                        \
        while (cnt--)                                                                                                  \
            __WRITE_DATA(color[total - cnt]);                                                                          \
    } while (0)


#ifdef __cplusplus
}
#endif





/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif