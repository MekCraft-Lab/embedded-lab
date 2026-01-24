/**
 *******************************************************************************
 * @file    app-asciiProcess.cpp
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

#include "app-asciiProcess.h"

#include <cstdlib>

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

uint8_t rxBuf[4][256]    = {0};

typedef struct {
    uint16_t distance_mm;
    uint16_t noise;
    uint16_t confidence;
} SensorData;
SensorData sensorData[4];



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "AsciiProcess"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static AsciiProcessApp asciProcessApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/

static char* find_substr(char* buf, size_t len, const char* key);
static const char* memmem_simple(const char* buf, size_t len, const char* key);
static int parse_distance_message(const char* buf, SensorData* , size_t len, size_t* used_len);


/* ------- function implement ----------------------------------------------------------------------------------------*/


AsciiProcessApp::AsciiProcessApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


AsciiProcessApp& AsciiProcessApp::instance() { return asciProcessApp; }


void AsciiProcessApp::init() {
    /* driver object initialize */

    HAL_UARTEx_ReceiveToIdle_DMA(&huart4, rxBuf[0], 64);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart5, rxBuf[1], 64);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxBuf[2], 64);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rxBuf[3], 64);
}


void AsciiProcessApp::run() {

    TickType_t t = xTaskGetTickCount();
    uint8_t* p_distance_message;
    uint16_t frame[5] = {0xA5, 0x00, 0x00, 0x00, 0x00};

    for (uint8_t i = 0; i < 4; i++) {
        size_t usedLen = 0;
        if (parse_distance_message(reinterpret_cast<const char*>(&rxBuf[i][0]), &sensorData[i], 0xFF, &usedLen));
        frame[i + 1] = sensorData[i].distance_mm;
    }

    HAL_UART_Transmit_DMA(&huart1, (uint8_t*)&frame[0], 10);
    vTaskDelayUntil(&t, 100);

}



uint8_t AsciiProcessApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t AsciiProcessApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }

void UART_Restart(UART_HandleTypeDef* huart) {
    HAL_UART_Abort(huart);
    HAL_Delay(10); // 给硬件缓冲器时间清空
    if (HAL_UART_Init(huart) != HAL_OK) {
        // 可在此添加日志或再次尝试
    }
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {

    // 1. 禁用 UART DMA
    HAL_UART_DMAStop(huart);

    // 2. 清除 UART 错误标志
    __HAL_UART_CLEAR_FEFLAG(huart);  // 帧错误
    __HAL_UART_CLEAR_NEFLAG(huart);  // 噪声错误
    __HAL_UART_CLEAR_OREFLAG(huart); // 溢出错误

    switch ((uint32_t)huart) {
        case UART4_BASE:
            HAL_UARTEx_ReceiveToIdle_DMA(&huart4, rxBuf[0], 64);

        case UART5_BASE:
            HAL_UARTEx_ReceiveToIdle_DMA(&huart5, rxBuf[1], 64);

        case USART3_BASE:
            HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxBuf[2], 64);

        case USART6_BASE:
            HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rxBuf[3], 64);
            break;
        default:
            // 其他 UART 可以单独处理
            break;
    }
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size) {

    switch ((uint32_t)huart->Instance) {


        case UART4_BASE: {


            HAL_UARTEx_ReceiveToIdle_DMA(&huart4, rxBuf[0], 64);


        } break;

        case UART5_BASE: {

            HAL_UARTEx_ReceiveToIdle_DMA(&huart5, rxBuf[1], 64);

        } break;

        case USART3_BASE: {
            HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxBuf[2], 64);
        } break;

        case USART6_BASE: {
            HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rxBuf[3], 64);
        }

        default: {
        }
    }
}


static char* find_substr(char* buf, size_t len, const char* key) {
    size_t key_len = strlen(key);
    for (size_t i = 0; i + key_len <= len; i++) {
        if (memcmp(buf + i, key, key_len) == 0) {
            return buf + i;
        }
    }
    return NULL;
}
static const char* memmem_simple(const char* buf, size_t len, const char* key) {
    size_t klen = strlen(key);
    if (len < klen)
        return NULL;

    for (size_t i = 0; i <= len - klen; i++) {
        if (memcmp(buf + i, key, klen) == 0)
            return buf + i;
    }
    return NULL;
}

int parse_distance_message(const char* buf, SensorData* data, size_t len, size_t* used_len) {
    const char* p = memmem_simple(buf, len, "Distance:");
    if (!p)
        return 0;

    const char* d_ptr = p + 9; // strlen("Distance:")
    const char* mm    = memmem_simple(d_ptr, len - (d_ptr - buf), "mm");
    if (!mm)
        return 0;

    const char* n_ptr = memmem_simple(mm, len - (mm - buf), "Noise:");
    if (!n_ptr)
        return 0;

    const char* c_ptr = memmem_simple(n_ptr, len - (n_ptr - buf), "Confidence:");
    if (!c_ptr)
        return 0;

    /* 提取数值 */
    data->distance_mm   = atoi(d_ptr);
    data->noise      = atoi(n_ptr + 6);  // strlen("Noise:")
    data->confidence = atoi(c_ptr + 11); // strlen("Confidence:")

    if (used_len) {
        /* 到 Confidence 数字结束即可 */
        const char* end = c_ptr + 11;
        while (end < buf + len && *end >= '0' && *end <= '9')
            end++;
        *used_len = end - buf;
    }

    return 1;
}