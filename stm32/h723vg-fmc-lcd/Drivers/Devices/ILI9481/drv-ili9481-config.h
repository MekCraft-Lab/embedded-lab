/**
 *******************************************************************************
 * @file    drv-ili9481-config.h
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

#ifndef VGT6_FMC_LCD_DRV_ILI9481_CONFIG_H
#define VGT6_FMC_LCD_DRV_ILI9481_CONFIG_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/




/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/
#define SCAN_FROM_TOP_TO_BOTTOM    0x00
#define SCAN_FROM_BOTTOM_TO_TOP    0x01
#define SCAN_FROM_EDGE_TO_MIDDLE   0x02
#define SCAN_FROM_MIDDLE_TO_EDGE   0x03

#define FRAME_RATE_125HZ           0x00
#define FRAME_RATE_100HZ           0x01
#define FRAME_RATE_85HZ            0x02
#define FRAME_RATE_72HZ            0x03
#define FRAME_RATE_56HZ            0x04
#define FRAME_RATE_50HZ            0x05
#define FRAME_RATE_45HZ            0x06
#define FRAME_RATE_42HZ            0x07



#define LANDSCAPE_MODE             0x00
#define PORTRAIT_MODE              0x01


#define COLOR_ORDER_RGB            0x00
#define COLOR_ORDER_BGR            0x01

#define REFRESH_FROM_TOP_TO_BOTTOM 0x00
#define REFRESH_FROM_BOTTOM_TO_TOP 0x01

#define PIXEL_FROM_3BIT            0x11
#define PIXEL_FROM_16BIT           0x55
#define PIXEL_FROM_18BIT           0x77



#define GRAYSCALE_INVERSION        0x00
#define SCAN_MODE                  SCAN_FROM_TOP_TO_BOTTOM
#define DISPLAY_LINES              480
#define SCAN_FPS                   FRAME_RATE_42HZ
#define DISPLAY_VERTICAL_FLIP      0x00
#define DISPLAY_HORIZONTAL_FLIP    0x00
#define DISPLAY_DIRECTION          LANDSCAPE_MODE
#define COLOR_ORDER                COLOR_ORDER_RGB
#define REFRESH_DIRECTION          REFRESH_FROM_TOP_TO_BOTTOM
#define PIXEL_FORMAT               PIXEL_FROM_16BIT


#define ADDR_MODE_PARAM                                                                                                \
    (DISPLAY_VERTICAL_FLIP | DISPLAY_HORIZONTAL_FLIP << 1 | COLOR_ORDER << 3 | REFRESH_DIRECTION << 4 |                \
     DISPLAY_DIRECTION << 5)





/*-------- 3. interface ----------------------------------------------------------------------------------------------*/




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*VGT6_FMC_LCD_DRV_ILI9481_CONFIG_H*/
