/**
 *******************************************************************************
 * @file    app-wirelessComm.cpp
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

#include "app-wirelessComm.h"

#include "spi.h"
#include "stm32h7xx_hal_spi.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static __attribute__((section("._dma_pool"))) uint8_t txBuff[512] = {0};
static __attribute__((section("._dma_pool"))) uint8_t rxBuff[512] = {0};




/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "WirelessComm"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static WirelessCommApp wirelessCommApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/




/* ------- function implement ----------------------------------------------------------------------------------------*/


WirelessCommApp::WirelessCommApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


WirelessCommApp& WirelessCommApp::instance() { return wirelessCommApp; }


void WirelessCommApp::init() {
    /* driver object initialize */

    uint8_t testBuf[] = {350 & 0xFF, 350 >> 8};

    memcpy(txBuff, testBuf, 2);

    HAL_SPI_Receive_DMA(&hspi1, rxBuff, sizeof(rxBuff));


}


void WirelessCommApp::run() {

    vTaskSuspend(nullptr);
}



uint8_t WirelessCommApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t WirelessCommApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

}

