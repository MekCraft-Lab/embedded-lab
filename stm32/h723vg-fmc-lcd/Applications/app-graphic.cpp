/**
 *******************************************************************************
 * @file    app-graphic.cpp
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
 * @date    2025/11/9
 * @version 1.0
 *******************************************************************************
 */


/* ------- define ----------------------------------------------------------------------------------------------------*/

#define ILI9841_BASE      (0x60000000UL)
#define ILI9841_CMD_ADDR  *(uint16_t*)(ILI9841_BASE)
#define ILI9841_DATA_ADDR *(uint16_t*)(0x60020000UL)

#define RS_PIN            GPIO_PIN_15
#define WR_PIN            GPIO_PIN_14
#define CS_PIN            GPIO_PIN_13
#define RST_PIN           GPIO_PIN_12


/* ------- include ---------------------------------------------------------------------------------------------------*/


/* I. header */
#include "app-graphic.h"


/* II. other application */
#include "app-host.h"

/* III. standard lib */
#include "gpio.h"

/* IV. utils */




/* ------- class prototypes-------------------------------------------------------------------------------------------*/

static void delayMs(uint32_t ms);




/* ------- macro -----------------------------------------------------------------------------------------------------*/

#define __LCD_GPIO_WRITE(x)                                                                                            \
    do {                                                                                                               \
        GPIOF->ODR = 0;                                                                                                \
        GPIOG->ODR = (uint16_t)x;                                                                                      \
        GPIOF->ODR = 0xFFFF;                                                                                           \
    } while (0);





/* ------- variables -------------------------------------------------------------------------------------------------*/

static ILI9481 lcd(delayMs);

uint32_t exec_time_us;


/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Graphic"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static GraphicApp graphicApp;

/* extern application */
static HostApp& host = HostApp::instance();




/* ------- message interface attribute -------------------------------------------------------------------------------*/

#define STREAM_BUFFER_SIZE 1536

static uint8_t sbStg[STREAM_BUFFER_SIZE];

static __attribute__((section(".dma_pool"))) uint8_t txBuf[512];




/* ------- function implement ----------------------------------------------------------------------------------------*/

GraphicApp::GraphicApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {
    _sbHandle = xStreamBufferCreateStatic(STREAM_BUFFER_SIZE, 1, sbStg, &_stm);
}

GraphicApp& GraphicApp::instance() { return graphicApp; }


void GraphicApp::init() {
    /* driver object initialize */
    host.waitInit();
    uint16_t ret;
    __WRITE_REG(ILI9481_CMD::SOFT_RESET);
    vTaskDelay(100);
    lcd.init();

    host.println("发送320x480x2个Byte执行时间：%d us, 折合%d MB/s", exec_time_us, (320 * 480 * 2) / exec_time_us );
}


void GraphicApp::run() {

    __WRITE_REG(ILI9481_CMD::ENTER_INVERT);
    vTaskDelay(1000);
    __WRITE_REG(ILI9481_CMD::EXIT_INVERT);
    vTaskDelay(1000);
}

uint8_t GraphicApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t GraphicApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


static void delayMs(uint32_t ms) { vTaskDelay(ms); }