/**
 *******************************************************************************
 * @file    app-createTimer.cpp
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
 * @date    2025/12/2
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-createTimer.h"

#include "Middlewares/lvgl/src/tick/lv_tick.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "CreateTimer"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static CreateTimerApp createTimerApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/






/* ------- function prototypes ---------------------------------------------------------------------------------------*/

static void lvglTickCb(TimerHandle_t xTimer) ;




/* ------- function implement ----------------------------------------------------------------------------------------*/


CreateTimerApp::CreateTimerApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 0, nullptr){
}


CreateTimerApp& CreateTimerApp::instance() {
    return createTimerApp;
}


void CreateTimerApp::init() {
    /* driver object initialize */

    _lvglTickTimer = xTimerCreate(
        "CreateTimer",
        pdMS_TO_TICKS(1),
        pdTRUE,
        NULL,
        lvglTickCb
        );

    if (_lvglTickTimer) {
        xTimerStart(_lvglTickTimer, 0);
    }


}


void CreateTimerApp::run() {
    vTaskDelete(NULL);
}



uint8_t CreateTimerApp::rxMsg(void* msg, uint16_t size) {

    return 0;
}

uint8_t CreateTimerApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) {

    return 0;
}


static void lvglTickCb(TimerHandle_t xTimer) {
    lv_tick_inc(1);
}