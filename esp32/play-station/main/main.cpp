/**
 *******************************************************************************
 * @file    main.cpp
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
 * @date    2025/12/11
 * @version 1.0
 *******************************************************************************
 */


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "lcd-i80.h"
#include "esp_lcd_panel_io.h"




/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/

#define __PANEL_SEND_CMD(x) esp_lcd_panel_io_tx_param(ioHandle,(uint8_t) x, nullptr, 0)

#define __PANEL_SEND_CMD_WITH_PARAM(cmd, param, len) esp_lcd_panel_io_tx_param(ioHandle, (uint8_t)cmd, (uint8_t[])param, len)





/* ------- variables -------------------------------------------------------------------------------------------------*/

esp_lcd_i80_bus_handle_t i80Bus;

esp_lcd_panel_io_handle_t ioHandle;

esp_lcd_panel_handle_t panelHandle;

TickType_t sysTick;





/* ------- function implement ----------------------------------------------------------------------------------------*/


extern "C" void app_main() {

    lcdInitBusIOAndPanel(&i80Bus, &ioHandle, &panelHandle);


    esp_lcd_panel_reset(panelHandle);

    vTaskDelay(100);
    printf("软复位完成\n");


    st7789Init();





    while (1) {
        sysTick = xTaskGetTickCount();

        esp_lcd_panel_io_tx_param(ioHandle, (uint8_t)ST7789Cmd::INVOFF, nullptr, 0);

        vTaskDelay(100);

        esp_lcd_panel_io_tx_param(ioHandle, (uint8_t)ST7789Cmd::INVON, nullptr, 0);

        vTaskDelay(100);

        printf("反转一次\n");






        vTaskDelayUntil(&sysTick, 1);
    }

    vTaskDelete(nullptr);
}