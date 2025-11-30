/**
 *******************************************************************************
 * @file    drv-ili9481.h
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

#ifndef VGT6_FMC_LCD_DRV_ILI9841_H
#define VGT6_FMC_LCD_DRV_ILI9841_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus
#include "drv-ili9481-cmd.h"
#include "drv-ili9481-config.h"

#endif



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#ifdef __cplusplus
enum class ILI9481_ERROR {
    NONE,
    OK,
    PARAM_ERR,
};
#endif


#define REG_ADDR  0x60000000
#define DATA_ADDR 0x60020000

/*-------- 3. interface ----------------------------------------------------------------------------------------------*/


#define __WRITE_REG(x)  *(__IO uint16_t*)REG_ADDR = (uint16_t)x;


#define __WRITE_DATA(x) *(__IO uint16_t*)DATA_ADDR = (uint16_t)x;


#define __START_AND_END_TO_BYTE_ARRAY(start, end)                                                                      \
    {((uint8_t)((start) >> 8)), ((uint8_t)((start) & 0xFF)), ((uint8_t)((end) >> 8)), ((uint8_t)((end) & 0xFF))}



#ifdef __cplusplus
using delayFunc = void (*)(uint32_t);


class ILI9481 {
public:
    ILI9481(delayFunc);
    static ILI9481_ERROR init();
    static ILI9481_ERROR test();
    static ILI9481_ERROR fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
    static void drawPixel(uint16_t x, uint16_t y, uint16_t color);


private:
    inline static delayFunc _delayMs = nullptr;
};

#endif



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*VGT6_FMC_LCD_DRV_ILI9841_H*/
