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

#define FRAME_SIZE 1204



/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-wirelessComm.h"

#include "Drivers/Devices/ILI9481/drv-ili9481-lvgl-adaptor.h"
#include "spi.h"
#include "stm32h7xx_hal_spi.h"

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static __attribute__((section("._dma_pool"))) uint8_t txBuff[512] = {0};
static __attribute__((section("._dma_pool"))) uint8_t rxBuff[512] = {0};

static __attribute__((section("._dma_pool"))) uint8_t frameBuffer[FRAME_SIZE + 2] = {0};



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

    _frameRxCplt = xSemaphoreCreateBinary();

    HAL_SPI_Receive_DMA(&hspi2, frameBuffer, sizeof(frameBuffer));


}

#include <cstdint>
#include <cstddef>

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void WirelessCommApp::run() {

    xSemaphoreTake(_frameRxCplt, portMAX_DELAY);

    size_t data_len = FRAME_SIZE; // 例如 20*30*BPP = 1200
    uint16_t received_crc = (frameBuffer[data_len] << 8) | frameBuffer[data_len + 1];

    // 校验 CRC
    uint16_t calc_crc = crc16_ccitt(frameBuffer, data_len);



    if (calc_crc != received_crc) {
        // CRC 错误，丢弃数据
        return;
    }

    // CRC 校验通过，解析坐标
    uint16_t positionX = frameBuffer[0] << 8 | frameBuffer[1];
    uint16_t positionY = frameBuffer[2] << 8 | frameBuffer[3];

    // 指向实际像素数据
    auto frame = reinterpret_cast<uint16_t*>(frameBuffer + 4);
    lcdFillRect(positionX, positionY, positionX + 19, positionY + 29, frame);
}



uint8_t WirelessCommApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t WirelessCommApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

}

extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi) {
    if (hspi == &hspi2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(wirelessCommApp._frameRxCplt, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        HAL_SPI_Receive_DMA(&hspi2, frameBuffer, sizeof(frameBuffer));
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)

{
    // 1. 禁用 SPI
    __HAL_SPI_DISABLE(hspi);

    // 2. 如果使用 DMA，需要先终止 DMA
    if(hspi->hdmarx != NULL)
    {
        HAL_DMA_Abort(hspi->hdmarx);
    }

    // 3. 清除错误标志
    __HAL_SPI_CLEAR_OVRFLAG(hspi);    // 如果可能溢出
    hspi->ErrorCode = HAL_SPI_ERROR_NONE;

    // 4. 重新初始化 SPI（保持原来的配置）
    HAL_SPI_Init(hspi);

    // 5. 如果使用 DMA接收，重新启动 DMA

    HAL_SPI_Receive_DMA(hspi, frameBuffer, FRAME_SIZE + 2);

    // 6. 使能 SPI
    __HAL_SPI_ENABLE(hspi);
}
