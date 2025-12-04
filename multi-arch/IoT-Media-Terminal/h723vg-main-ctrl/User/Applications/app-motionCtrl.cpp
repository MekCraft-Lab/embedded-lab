/**
 *******************************************************************************
 * @file    app-motionCtrl.cpp
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
 * @date    2025/12/4
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/

#define SQRT3_DIV_2    0.8660254037844386f


#define BODY_RADIUS    0.5f // m

#define WHEEL_RADIUS   0.05f // m

#define TRIGGER_RATIO  (0.8f / 0x3FF)

#define JOYSTICK_RATIO (1.57f / 0x3FF)

#define MAX_SPEED      (160 * M_PI / 60)

/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-motionCtrl.h"

#include "spi.h"
#include "stm32h7xx_hal_tim.h"
#include "tim.h"
#include "usart.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

float testKp = (float)10000 / MAX_SPEED;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim15;
struct {
    struct {
        float vx; // m/s 前
        float vy; // m/s 左
        float wz; // rad/s 右手系
        float theta;
    } reference;

    struct {
        float vx;
        float vy;
        float wz;
        float theta;
    } feedback;

    struct {
        TIM_HandleTypeDef* htim;
        uint8_t channel;
        GPIO_TypeDef* port1;
        uint16_t pin1;

        GPIO_TypeDef* port2;
        uint16_t pin2;
    } ctrlChannel[4] = {{.htim    = &htim2,
                         .channel = TIM_CHANNEL_1,
                         .port1   = GPIOE,
                         .pin1    = GPIO_PIN_4,
                         .port2   = GPIOE,
                         .pin2    = GPIO_PIN_5},
                        {.htim    = &htim2,
                         .channel = TIM_CHANNEL_2,
                         .port1   = GPIOE,
                         .pin1    = GPIO_PIN_6,
                         .port2   = GPIOC,
                         .pin2    = GPIO_PIN_13},
                        {.htim    = &htim2,
                         .channel = TIM_CHANNEL_3,
                         .port1   = GPIOC,
                         .pin1    = GPIO_PIN_4,
                         .port2   = GPIOC,
                         .pin2    = GPIO_PIN_5},
                        {.htim    = &htim2,
                         .channel = TIM_CHANNEL_4,
                         .port1   = GPIOC,
                         .pin1    = GPIO_PIN_3,
                         .port2   = GPIOC,
                         .pin2    = GPIO_PIN_2}};

    struct {
        TIM_HandleTypeDef* htim;
        uint8_t channel;
    } measureChannel[4] = {{.htim = &htim1, .channel = TIM_CHANNEL_2},   // PA9
                           {.htim = &htim3, .channel = TIM_CHANNEL_1},   // PC6
                           {.htim = &htim8, .channel = TIM_CHANNEL_2},   // PC7
                           {.htim = &htim15, .channel = TIM_CHANNEL_2}}; // PA3

    struct {
        uint16_t lastEncode;
        uint16_t encodeInc;
        uint32_t lastTick;
        uint32_t tickInc;
        float rawSpeedRps;
        float reducedSpeedRps;
        float reducedSpeedRad;
        float refRad;
    } wheelData[4];

    uint16_t output[4];

} motionData;


__attribute__((section("._dma_pool"))) __packed struct {
    uint16_t leftJoyStickX;
    uint16_t leftJoyStickY;
    uint16_t rightJoyStickX;
    uint16_t rightJoyStickY;

    uint16_t leftTrigger; // 最大0x03FF
    uint16_t rightTrigger;

    uint8_t dpad;
    uint32_t L1       : 1;
    uint32_t R1       : 1;
    uint32_t reverse  : 1;
    uint32_t buttonY  : 1;

    uint32_t buttonX  : 1;
    uint32_t reverse2 : 1;
    uint32_t buttonB  : 1;
    uint32_t buttonA  : 1;

    uint32_t reverse3 : 1;
    uint32_t rPress   : 1;
    uint32_t lPress   : 1;
    uint32_t reverse4 : 4;

    uint32_t Home     : 1;

    uint32_t menu     : 1;
    uint32_t capture  : 1;
    uint32_t reverse5 : 5;
    uint32_t start    : 1;
} gamePad = {
    .leftJoyStickX = 0x8000,
    .leftJoyStickY = 0x7FFF,
    .rightJoyStickX = 0x8000,
    .rightJoyStickY = 0x7FFF,
};

uint8_t* pGamePad = reinterpret_cast<uint8_t*>(&gamePad);
/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "MotionCtrl"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static MotionCtrlApp motionCtrlApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


MotionCtrlApp::MotionCtrlApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


MotionCtrlApp& MotionCtrlApp::instance() { return motionCtrlApp; }


void MotionCtrlApp::init() {
    /* driver object initialize */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart8, reinterpret_cast<uint8_t*>(&gamePad), sizeof(gamePad));
    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2 | TIM_CHANNEL_3 | TIM_CHANNEL_4);
    TIM2->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E; // CH1 输出使能

}


int rx ;
int ry ;
int lx ;
int ly ;

void MotionCtrlApp::run() {

    uint32_t startTick      = TIM24->CNT;
    TickType_t sysTick      = xTaskGetTickCount();



    float total             = (gamePad.rightTrigger - gamePad.leftTrigger) * TRIGGER_RATIO;

    rx                  = 0x8000 - gamePad.rightJoyStickX;
    ry                  = 0x7FFF - gamePad.rightJoyStickY;
    lx                  = 0x8000 - gamePad.leftJoyStickX;
    ly                  = 0x7FFF - gamePad.leftJoyStickY;

    float c                 = rx * rx + ry * ry;

    if (c == 0) {
        motionData.reference.vx = total;
        motionData.reference.vy = 0;

    } else {
        motionData.reference.vy = (float)rx / sqrt(c) * total;

        motionData.reference.vx = (float)ry / sqrt(c) * total;
    }



    for (uint8_t i = 0; i < 4; i++) {
        motionData.wheelData[i].encodeInc =
            (motionData.measureChannel[i].htim->Instance->CNT - motionData.wheelData[i].lastEncode);
        motionData.wheelData[i].lastEncode = motionData.measureChannel[i].htim->Instance->CNT;
        motionData.wheelData[i].tickInc    = TIM24->CNT - motionData.wheelData[i].lastTick;
        motionData.wheelData[i].lastTick   = TIM24->CNT;


        motionData.wheelData[i].rawSpeedRps =
            (static_cast<float>(motionData.wheelData[i].encodeInc) / motionData.wheelData[i].tickInc) * 1000;
        motionData.wheelData[i].reducedSpeedRps = motionData.wheelData[i].rawSpeedRps / 440;
        motionData.wheelData[i].reducedSpeedRad = motionData.wheelData[i].reducedSpeedRad / (2 * M_PI);
    }





    motionData.wheelData[0].refRad = (0.5f * motionData.reference.vx - SQRT3_DIV_2 * motionData.reference.vy -
                                      motionData.reference.wz * BODY_RADIUS) /
                                     WHEEL_RADIUS;
    motionData.wheelData[1].refRad = (0.5f * motionData.reference.vx + SQRT3_DIV_2 * motionData.reference.vy -
                                      motionData.reference.wz * BODY_RADIUS) /
                                     WHEEL_RADIUS;
    motionData.wheelData[2].refRad = (-0.5f * motionData.reference.vx + SQRT3_DIV_2 * motionData.reference.vy -
                                      motionData.reference.wz * BODY_RADIUS) /
                                     WHEEL_RADIUS;
    motionData.wheelData[3].refRad = (-0.5f * motionData.reference.vx - SQRT3_DIV_2 * motionData.reference.vy -
                                      motionData.reference.wz * BODY_RADIUS) /
                                     WHEEL_RADIUS;




    for (uint8_t i = 0; i < 4; i++) {

        if (motionData.wheelData[i].refRad < 0) {
            motionData.ctrlChannel[i].port1->BSRR = motionData.ctrlChannel[i].pin1;
            motionData.ctrlChannel[i].port2->BSRR = motionData.ctrlChannel[i].pin2 << 16;
            motionData.wheelData->reducedSpeedRad = motionData.wheelData->reducedSpeedRad * -1;
        } else {
            motionData.ctrlChannel[i].port1->BSRR = motionData.ctrlChannel[i].pin1 << 16;
            motionData.ctrlChannel[i].port2->BSRR = motionData.ctrlChannel[i].pin2;
        }

        if (abs(motionData.wheelData[i].refRad) > 8.373) {
            motionData.wheelData[i].refRad = 8.373;
        }

        motionData.output[i] = testKp * abs(motionData.wheelData[i].refRad);

        __HAL_TIM_SET_COMPARE(motionData.ctrlChannel[i].htim, motionData.ctrlChannel[i].channel, motionData.output[i]);
    }




    uint32_t endTick = TIM24->CNT;
    vTaskDelayUntil(&sysTick, 5);
}



uint8_t MotionCtrlApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t MotionCtrlApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }
