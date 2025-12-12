/**
 *******************************************************************************
 * @file    app-nofrendo.cpp
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
 * @date    2025/12/12
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/




/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-nofrendo.h"

#include "app-filesys.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "lcd-i80.h"
#include "nofrendo.h"

#include <cstring>

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Nofrendo"

#define APPLICATION_STACK_SIZE 8192

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static NofrendoApp nofrendoApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/



/*------- 应用建立 ---------*/

NofrendoApp::NofrendoApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


NofrendoApp& NofrendoApp::instance() { return nofrendoApp; }


void nvs_flash_init();




void NofrendoApp::init() {

    FilesysApp::instance().waitInit();

    printf("NoFrendo start!\n");
    nofrendo_main(0, nullptr);
    ESP_LOGE("Nofrendo", "NoFrendo died? WtF?\n");
    vTaskDelete(nullptr);
}


/**
 * @brief 主循环任务，用于刷新LCD
 */
void NofrendoApp::run() {

    vTaskDelete(nullptr);
}



uint8_t NofrendoApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t NofrendoApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }

