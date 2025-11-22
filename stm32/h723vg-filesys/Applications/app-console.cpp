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
 * @date    2025/8/28
 * @version 1.0
 *******************************************************************************
 */


/* ------- define --------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "app-console.h"

#include <cstdarg>

#include "app-shell.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/

extern "C" {
void cdcReceiveCallback(uint8_t* data, uint16_t len);
}
/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_NAME       "Console"

#define APPLICATION_STACK_SIZE 256

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static ConsoleApp consoleApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/

#define RING_BUFFER_SIZE 1024

static uint8_t rbStg1[RING_BUFFER_SIZE];
static uint8_t rbStg2[RING_BUFFER_SIZE];

#define QUEUE_LEN 8

static uint8_t qStg[QUEUE_LEN * sizeof(BufferMsg)];



#define FORMAT_BUF_LEN 1024

static char formatBuf[1024];



/* ------- function implement ----------------------------------------------------------------------------------------*/


ConsoleApp::ConsoleApp():StaticAppBase(APPLICATION_NAME, APPLICATION_STACK_SIZE, APPLICATION_PRIORITY, appStack),
_rb1(rbStg1, RING_BUFFER_SIZE, &_queue), _rb2(rbStg2, RING_BUFFER_SIZE, &_queue){
    _queue = xQueueCreateStatic(QUEUE_LEN, sizeof(BufferMsg), qStg, &_staticQueue);

}

ConsoleApp& ConsoleApp::instance() {
    return consoleApp;
}



void ConsoleApp::init() {
    _comm     = p_cdc_fcty->produce();
    _logQueue = xQueueCreate(16, sizeof(LogMsg));
    configASSERT(_logQueue);

}


void ConsoleApp::run() {
    // LogMsg logMsg;
    //
    // if (xQueueReceive(_logQueue, &logMsg, portMAX_DELAY) == pdPASS) {
    //     _comm->transmit((uint8_t*)logMsg.getMsg(), logMsg.getMsgLen());
    // }

    BufferMsg msg;
    RingBuffer* rb[2] = {&_rb1, &_rb2};


    // when you process rb2, the new message will push into rb1
    xQueueReceive(_queue, &msg, portMAX_DELAY);


    // so when the rb2 is empty, and the new message come, then you should change the ring buffer.
    if (rb[_index]->getSize() == 0) {
        _index = !_index;
    }

    uint16_t offset = msg.offset;
    uint16_t len = msg.len;

    uint8_t* pData = reinterpret_cast<uint8_t *>(reinterpret_cast<uint32_t>(rb[_index]->getBase()) + offset);

    if (offset + len > rb[_index]->getCapacity()) {
        uint16_t thisLen = rb[_index]->getCapacity() - offset;
        uint16_t nextLen = len - thisLen;
        _comm->transmit(pData,thisLen);
        _comm->transmit(rb[_index]->getBase(), nextLen);
        rb[_index]->tailIncrease(len);
    } else {
        _comm->transmit(pData, len);
        rb[_index]->tailIncrease(len);
    }

}

void ConsoleApp::println(const char *fmt, ...) {

    va_list vargs;
    int len;

    va_start(vargs, fmt);
    len = vsnprintf(formatBuf, FORMAT_BUF_LEN - 1, fmt, vargs);
    va_end(vargs);

    if (len > FORMAT_BUF_LEN)
    {
        len = FORMAT_BUF_LEN;
    }

    formatBuf[len++] = '\r';
    formatBuf[len++] = '\n';
    formatBuf[len] = 0;

    output(formatBuf, len);

}

void ConsoleApp::error(const char *fmt, ...) {

    va_list vargs;
    int len;

    va_start(vargs, fmt);
    memcpy(formatBuf, "ERROR: ", 7);
    len = vsnprintf(formatBuf + 7, FORMAT_BUF_LEN - 1, fmt, vargs) + 7;
    va_end(vargs);

    if (len > FORMAT_BUF_LEN)
    {
        len = FORMAT_BUF_LEN;
    }

    formatBuf[len++] = '\r';
    formatBuf[len++] = '\n';
    formatBuf[len] = 0;

    output(formatBuf, len);
}

void ConsoleApp::output(void *data, const uint16_t len) {
    // LogMsg logMsg(static_cast<const char*>(data), len);
    // xQueueSend(_logQueue, &logMsg, portMAX_DELAY);

    RingBuffer* rb[2] = { &_rb1, &_rb2};
    rb[!_index]->push((const uint8_t *)data, len, portMAX_DELAY);
}

void cdcReceiveCallback(uint8_t* data, const uint16_t len) {
    // rxLen = len;
    // memcpy(rxBuffer, data, len);
    // xQueueSend(ShellApp::instance()._rxQueue, &rxLen, portMAX_DELAY);
    ShellApp::instance().sendMsg(data, len);
}