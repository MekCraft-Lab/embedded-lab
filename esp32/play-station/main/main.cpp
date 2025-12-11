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

#define TAG "main"

#define DISP_WIDTH 240
#define DISP_HEIGHT 240


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "applications/app-gamepad.h"
#include "demos/benchmark/lv_demo_benchmark.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lcd-i80.h"

#include "applications/application-base.h"



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

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);


    static lv_disp_t * disp_handle;

    /* LCD IO */
    lcdInitBusIOAndPanel(&i80Bus, &ioHandle, &panelHandle);

    esp_lcd_panel_reset(panelHandle);

    st7789Init();


    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = ioHandle,
        .panel_handle = panelHandle,
        .buffer_size = DISP_WIDTH * 20 * 2,
        .double_buffer = true,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }
    };

    disp_handle = lvgl_port_add_disp(&disp_cfg);

    /* ... the rest of the initialization ... */


    if (lvgl_port_lock(30)) {

        lv_demo_benchmark();

        lvgl_port_unlock();
    }


    GamepadApp::instance();

    StaticAppBase::startApplications();



    while (1) {


        vTaskDelayUntil(&sysTick, 1);
    }

    vTaskDelete(nullptr);
}