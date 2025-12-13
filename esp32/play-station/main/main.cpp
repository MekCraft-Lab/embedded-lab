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

#define TAG         "main"




/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "applications/app-audio.h"
#include "applications/app-display.h"
#include "applications/app-filesys.h"
#include "applications/app-gamepad.h"
#include "applications/app-nofrendo.h"
#include "demos/benchmark/lv_demo_benchmark.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "applications/application-base.h"



/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/

#define __PANEL_SEND_CMD(x) esp_lcd_panel_io_tx_param(ioHandle, (uint8_t)x, nullptr, 0)

#define __PANEL_SEND_CMD_WITH_PARAM(cmd, param, len)                                                                   \
    esp_lcd_panel_io_tx_param(ioHandle, (uint8_t)cmd, (uint8_t[])param, len)





/* ------- variables -------------------------------------------------------------------------------------------------*/



TickType_t sysTick;





/* ------- function implement ----------------------------------------------------------------------------------------*/

char buffer[512];
uint8_t checkFlag = 0;
extern uint32_t fps;
extern "C" void app_main() {

    GamepadApp::instance();
    NofrendoApp::instance();

    FilesysApp::instance();
    // AudioApp::instance();
    DisplayApp::instance();

    StaticAppBase::startApplications();



    for (;;) {


        static uint32_t lastFlag = 0;

        lastFlag = fps;

        vTaskDelay(500);
        //
        // vTaskList(buffer);
        // printf("%s", buffer);
    }

    vTaskDelete(NULL);

}