/**
 *******************************************************************************
 * @file    app-console.cpp
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
 * @date    2025/8/21
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */
#include "app-host.h"

/* II. other application */
#include "./app-fileSystem.h"

/* III. standard lib */
#include "app-wirelessComm.h"


#include <cstdarg>
#include <cstdio>
#include <cstring>




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Console"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static HostApp hostApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/

#define STREAM_BUFFER_SIZE 1536

static uint8_t sbStg[STREAM_BUFFER_SIZE];


__attribute__((section("._dma_pool"))) uint8_t txBuf[512];
uint8_t frameBuildBuffer[512];
uint8_t responseBuffer[512];


/* ------- function prototypes ---------------------------------------------------------------------------------------*/

void UartSend(void*, uint16_t);


/* ------- function implement ----------------------------------------------------------------------------------------*/


HostApp::HostApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 0, nullptr){

    _sbHandle = xStreamBufferCreateStatic(STREAM_BUFFER_SIZE, 1, sbStg, &_stm);
}

void HostApp::init() {
    /* driver object initialize */
    WirelessCommApp::instance().waitInit();
    _waitForTransmitLock = xSemaphoreCreateBinary();
    println("console application start");
}


void HostApp::run() {
    // 阻塞直到消息到来，先读取长度头（假设每条消息前有 2 字节长度）
    uint16_t msg_len;
    xStreamBufferReceive(_sbHandle, &msg_len, sizeof(msg_len), portMAX_DELAY);

    // 然后动态申请缓冲区接收完整消息
    // void *msg_buf = pvPortMalloc(msg_len);
    xStreamBufferReceive(_sbHandle, txBuf, msg_len, portMAX_DELAY);
    UartSend(txBuf, msg_len);
    // vPortFree(msg_buf);
    xSemaphoreTake(_waitForTransmitLock, portMAX_DELAY);

}



uint8_t HostApp::rxMsg(void* msg, uint16_t size) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xStreamBufferSendFromISR(_sbHandle, &size, 2, &xHigherPriorityTaskWoken);
    xStreamBufferSendFromISR(_sbHandle, msg, size, &xHigherPriorityTaskWoken);

    return 0;
}

bool HostApp::println(const char* fmt, ...) {
    va_list vargs;
    char formatBuf[512];

    va_start(vargs, fmt);
    uint16_t len = vsnprintf(formatBuf, 500, fmt, vargs);
    va_end(vargs);

    if (len > 500) {
        len = 500;
    }

    formatBuf[len++] = '\r';
    formatBuf[len++] = '\n';
    formatBuf[len]   = 0;

    bool ret         = rxMsg(formatBuf, len , portMAX_DELAY);
    return ret;
}

bool HostApp::println(uint8_t ISR, const char* fmt, ...) {
    va_list vargs;
    char formatBuf[512];

    va_start(vargs, fmt);
    uint16_t len = vsnprintf(formatBuf, 500, fmt, vargs);
    va_end(vargs);

    if (len > 500) {
        len = 500;
    }

    formatBuf[len++] = '\r';
    formatBuf[len++] = '\n';
    formatBuf[len]   = 0;

   bool ret         = rxMsg(formatBuf, len);
    return ret;
}

uint8_t HostApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) {
    xStreamBufferSend(_sbHandle, &size, 2, timeout);
    xStreamBufferSend(_sbHandle, msg, size, timeout);
    return 0;
}

HostApp& HostApp::instance() {
    return hostApp;
}

void HostApp::frameResp(MekFrame& f) {
    MekProtocolHeader header = {};
    header.sof = 0xAA55;
    header.seq = f.seq;
    header.cmd = f.type;
    header.len = f.payload.size();
    memcpy(frameBuildBuffer, &header, sizeof(header));
    memcpy(frameBuildBuffer + sizeof(header), f.payload.data(), f.payload.size());

    rxMsg(frameBuildBuffer, sizeof(header) + f.payload.size() + 2, portMAX_DELAY);
}



void UartSend(void* pData, uint16_t size) {
    // memcpy(txBuf, pData, size);
    HAL_UART_Transmit_DMA(&huart1, txBuf, size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    if (huart == &huart1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(hostApp._waitForTransmitLock, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}



extern "C" void hostPrintln(const char* fmt, ...) {
    va_list vargs;
    char formatBuf[512];

    va_start(vargs, fmt);
    uint16_t len = vsnprintf(formatBuf, 500, fmt, vargs);
    va_end(vargs);

    if (len > 500) {
        len = 500;
    }

    formatBuf[len++] = '\r';
    formatBuf[len++] = '\n';
    formatBuf[len]   = 0;

    hostApp.rxMsg(formatBuf, len, portMAX_DELAY);

}