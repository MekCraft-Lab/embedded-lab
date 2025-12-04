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





/* ------- include ---------------------------------------------------------------------------------------------------*/


/* I. header */
#include "app-graphic.h"


/* II. other application */
#include "app-host.h"

/* III. standard lib */
#include "Middlewares/lvgl/lv_port_disp.h"
#include "Middlewares/lvgl/src/lv_init.h"
#include "app-createTimer.h"
#include "gpio.h"
#include "tim.h"

#include <cstdio>
#include <ctime>

/* IV. utils */




/* ------- class prototypes-------------------------------------------------------------------------------------------*/



void lv_demo_clock(void);


/* ------- macro -----------------------------------------------------------------------------------------------------*/

#define __LCD_GPIO_WRITE(x)                                                                                            \
    do {                                                                                                               \
        GPIOF->ODR = 0;                                                                                                \
        GPIOG->ODR = (uint16_t)x;                                                                                      \
        GPIOF->ODR = 0xFFFF;                                                                                           \
    } while (0);





/* ------- variables -------------------------------------------------------------------------------------------------*/

static ILI9481 lcd(GraphicApp::delayMs);

uint32_t exec_time_us;




/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Graphic"

#define APPLICATION_STACK_SIZE 2048

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

    CreateTimerApp::instance().waitInit();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    vTaskDelay(500);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    vTaskDelay(500);
    uint16_t ret;
    __WRITE_REG(ILI9481_CMD::SOFT_RESET);
    vTaskDelay(100);

    lv_init();

    lv_port_disp_init();

    lv_demo_clock();

    HAL_TIM_Base_Start(&htim24);

    // 创建全屏图片对象
    lv_obj_t *video_img = lv_img_create(lv_scr_act());
    lv_obj_align(video_img, LV_ALIGN_CENTER, 0, 0);




}


void GraphicApp::run() {



    TickType_t last_wake_up = xTaskGetTickCount();
    uint32_t startTick = TIM24->CNT;
    lv_timer_handler();

    uint32_t execTime = TIM24->CNT - startTick;
    vTaskDelayUntil(&last_wake_up, 1000);

}

uint8_t GraphicApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t GraphicApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


void GraphicApp::delayMs(uint32_t ms) { vTaskDelay(ms); }



static lv_obj_t *label_clock;

static void clock_update_cb(lv_timer_t *timer)
{
    (void)timer;

    /* 使用实际 RTC 或系统时间 */
    uint32_t now = xTaskGetTickCount();

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             now / 1000 / 60 / 60, (now / 1000 / 60) % 60, (now / 1000) % 60);

    lv_label_set_text(label_clock, buf);
}

void lv_demo_clock(void)
{
    label_clock = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_clock, &lv_font_montserrat_48, 0);
    lv_obj_center(label_clock);

    /* 每 1000ms 更新一次 */
    lv_timer_create(clock_update_cb, 1000, NULL);
}


