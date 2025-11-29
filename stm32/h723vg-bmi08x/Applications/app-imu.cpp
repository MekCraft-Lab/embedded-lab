/**
 *******************************************************************************
 * @file    app-imu.cpp
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
 * @date    2025/11/24
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-imu.h"

#include "tim.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static __attribute__((section(".dma_pool"))) uint8_t rxBuf[64];
static __attribute__((section(".dma_pool"))) uint8_t txBuf[64];
static __attribute__((section(".dma_pool"))) uint8_t range;
static __attribute__((section(".dma_pool"))) uint16_t temp1;
typedef union {
    struct __attribute__((packed)) {
        uint8_t lsb; // 最低的 8 bits
        uint8_t msb; // 高 3 bits（位7~5）
    };
    struct __attribute__((packed)) {
        int16_t temp11 : 11; // 11bit signed
    };
} temp_raw11_t;

static __attribute__((section(".dma_pool"))) temp_raw11_t temp_raw11;


/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Imu"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static ImuApp imuApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/

bool waitForReceive(uint32_t timeout);

bool waitForTransmit(uint32_t timeout);
bool waitForTransmitReceive(uint32_t timeout);




/* ------- function implement ----------------------------------------------------------------------------------------*/


ImuApp::ImuApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr),
      _bmi088(&hspi2, txBuf, rxBuf, waitForTransmit, waitForReceive, waitForTransmitReceive, GPIOC, GPIO_PIN_0, GPIOC,
              GPIO_PIN_3) {}


ImuApp& ImuApp::instance() { return imuApp; }



void ImuApp::init() {

    _waitForReceive         = xSemaphoreCreateBinary();
    _waitForTransmit        = xSemaphoreCreateBinary();
    _waitForTransmitReceive = xSemaphoreCreateBinary();

    _bmi088.readGyroIDAsync();
    xSemaphoreTake(_waitForTransmitReceive, portMAX_DELAY);
    _bmi088.asyncRxCallback();

    HAL_TIM_Base_Start(&htim24);


    GPIOC->BSRR = GPIO_PIN_0;

    vTaskDelay(10);
    GPIOC->BSRR = GPIO_PIN_0 << 16;
    vTaskDelay(10);
    GPIOC->BSRR = GPIO_PIN_0;


    _bmi088.readRegister(Bmi088Chip::GYRO, 0x0F, &range, 1, portMAX_DELAY);
    _bmi088.readAccIDAsync();
    xSemaphoreTake(_waitForTransmitReceive, portMAX_DELAY);
    _bmi088.asyncRxCallback();

    uint8_t temp[10] = {0};

    _bmi088.readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_PWR_CONF, temp, 3, portMAX_DELAY);
    vTaskDelay(10);
    _bmi088.readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_PWR_CTRL, temp + 3, 3, portMAX_DELAY);
    vTaskDelay(10);

    temp[0] = 0x00;
    _bmi088.writeRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_PWR_CONF, temp, 1, portMAX_DELAY);
    vTaskDelay(10);
    temp[0] = 0x04;
    _bmi088.writeRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_PWR_CTRL, temp, 1, portMAX_DELAY);
    vTaskDelay(10);

    _bmi088.readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_PWR_CONF, temp, 3, portMAX_DELAY);
    vTaskDelay(10);
    _bmi088.readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_PWR_CTRL, temp + 3, 3, portMAX_DELAY);
    vTaskDelay(10);
    _bmi088.readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::TEMPERATURE_SENSOR_DATA, temp, 4, portMAX_DELAY);



    temp_raw11.lsb = temp[2];
    temp_raw11.msb = temp[3];

    _bmi088.readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_RANGE, temp, 3, portMAX_DELAY);
    vTaskDelay(10);
    _bmi088.readRegister(Bmi088Chip::GYRO, (uint8_t)Bmi088GyroRegister::GYRO_RANGE, temp + 3, 2, portMAX_DELAY);

    float temperature = (float)temp_raw11.temp11 * 0.125 + 23;
}

float accelXinMg;
float accelZinMg;
float accelYinMg;
float gyroX;
float gyroZ;
float gyroY;

float exeTimeUs;
float lastTime;
void ImuApp::run() {
    uint32_t lastTime  = TIM24->CNT;
    TickType_t sysTick = xTaskGetTickCount();



    int16_t ret[6];
    _bmi088.updateGyroAndAccSync(ret, portMAX_DELAY);

    gyroX      = (float)ret[0] / 0x7FFF * 125;
    gyroY      = (float)ret[1] / 0x7FFF * 125;
    gyroZ      = (float)ret[2] / 0x7FFF * 125;

    accelXinMg = (float)ret[3] / 0x8000 * 1.5 * 4;
    accelYinMg = (float)ret[4] / 0x8000 * 1.5 * 4;
    accelZinMg = (float)ret[5] / 0x8000 * 1.5 * 4;

    vTaskDelayUntil(&sysTick, 1);
    exeTimeUs = TIM24->CNT - lastTime;
}



uint8_t ImuApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t ImuApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2 && imuApp._inited) {
        GPIOC->BSRR                         = (GPIO_PIN_0 | GPIO_PIN_3);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(imuApp._waitForReceive, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2 && imuApp._inited) {
        GPIOC->BSRR                         = (GPIO_PIN_0 | GPIO_PIN_3);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(imuApp._waitForTransmit, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2 && imuApp._inited) {
        GPIOC->BSRR                         = (GPIO_PIN_0 | GPIO_PIN_3);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(imuApp._waitForTransmitReceive, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

bool waitForReceive(uint32_t timeout) { return xSemaphoreTake(imuApp._waitForReceive, timeout); }

bool waitForTransmit(uint32_t timeout) { return xSemaphoreTake(imuApp._waitForTransmit, timeout); }

bool waitForTransmitReceive(uint32_t timeout) { return xSemaphoreTake(imuApp._waitForTransmitReceive, timeout); }
