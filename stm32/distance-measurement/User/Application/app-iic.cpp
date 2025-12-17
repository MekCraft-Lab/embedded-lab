/**
 *******************************************************************************
 * @file    app-iic.cpp
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
 * @date    2025/12/17
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-iic.h"

#include "i2c.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static TickType_t appTicks;

uint8_t iic1_rxbuf[16];



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Iic"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static IicApp iicApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/






/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


IicApp::IicApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 0, nullptr){
}


IicApp& IicApp::instance() {
    return iicApp;
}


void IicApp::init() {
    /* driver object initialize */

}


void IicApp::run() {

    appTicks = xTaskGetTickCount();


    //HAL_I2C_Mem_Read_IT(&hi2c1, 0x6C, 0x03, I2C_MEMADD_SIZE_8BIT, iic1_rxbuf, 8);
    uint8_t addr = 0x03;
    HAL_I2C_Master_Transmit_DMA(&hi2c1, 0xD8, &addr, 1);
    xSemaphoreTake(_txCpltSemaphore, portMAX_DELAY);
    HAL_I2C_Master_Receive_DMA(&hi2c1, 0xD9, &iic1_rxbuf[0], 16);

    uint16_t distance = iic1_rxbuf[0] | iic1_rxbuf[1] << 8;


    vTaskDelayUntil(&appTicks, 100);
 
}



uint8_t IicApp::rxMsg(void* msg, uint16_t size) {

    return 0;
}

uint8_t IicApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) {

    return 0;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef* hi2c) {
    if (hi2c == &hi2c1) {
        BaseType_t pxHigher;
        xSemaphoreGiveFromISR(IicApp::instance()._txCpltSemaphore, &pxHigher);

    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) {
    if (hi2c == &hi2c1) {
        BaseType_t pxHigher;
        xSemaphoreGiveFromISR(IicApp::instance()._txCpltSemaphore, &pxHigher);

    }
}

