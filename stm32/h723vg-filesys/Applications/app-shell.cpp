/**
 *******************************************************************************
 * @file    app-shell.cpp
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

#define SHELL_GET_TICK() xTaskGetTickCount()


/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */
#include "app-shell.h"

/* II. other application */
#include "app-console.h"

/* III. standard lib */
#include <cstring>

#include "app-file.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/


static char shellBuffer[512];
static char logBuffer[512];
static uint8_t rxBuffer[64];
static uint16_t rxLen;



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_NAME       "Shell"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static ShellApp shellApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/

#define RING_BUFFER_SIZE 64

static uint8_t rbStg1[RING_BUFFER_SIZE];
static uint8_t rbStg2[RING_BUFFER_SIZE];

#define QUEUE_LEN 8

static uint8_t qStg[QUEUE_LEN * sizeof(BufferMsg)];



/* ------- function prototypes ---------------------------------------------------------------------------------------*/


int16_t shellWrite(char* data, uint16_t len);
void logWrite(char* data, int16_t len);
void cdcReceiveCallback(uint8_t* data, uint16_t len);




/* ------- function implement ----------------------------------------------------------------------------------------*/


ShellApp::ShellApp()
    : StaticAppBase(APPLICATION_NAME, APPLICATION_STACK_SIZE, APPLICATION_PRIORITY, appStack),
      _rb1(rbStg1, RING_BUFFER_SIZE, &_queue), _rb2(rbStg2, RING_BUFFER_SIZE, &_queue) {
    _queue = xQueueCreateStatic(QUEUE_LEN, sizeof(BufferMsg), qStg, &_staticQueue);
}

ShellApp& ShellApp::instance() {
    return shellApp;
}


void ShellApp::init() {
    /* driver object initialize */
    _shell.write = shellWrite;
    _log.write   = logWrite;
    _log.active  = true;
    _log.level   = LOG_DEBUG;

    ConsoleApp::instance().waitInit();

    shellInit(&_shell, shellBuffer, 512);
    logRegister(&_log, &_shell);
    shellHandler(&_shell, rxBuffer[0]);
}



void ShellApp::run() {
    BufferMsg msg;
    RingBuffer* rb[2] = {&_rb1, &_rb2};


    // when you process rb2, the new message will push into rb1
    xQueueReceive(_queue, &msg, portMAX_DELAY);


    // so when the rb2 is empty, and the new message come, then you should change the ring buffer.
    if (rb[_index]->getSize() == 0) {
        _index = !_index;
    }

    for (uint16_t i = 0; i < msg.len; i++) {
        shellHandler(&_shell, rb[_index]->getData());
    }

}


int16_t shellWrite(char* data, const uint16_t len) {
    ConsoleApp::instance().output(data, len);
    return len;
}

void logWrite(char* data, int16_t len) {
    if (ShellApp::instance()._log.shell) {
        shellWriteEndLine(ShellApp::instance()._log.shell, data, len);
    } else {
        ConsoleApp::instance().output(data, len);
    }
}

void ShellApp::sendMsg(uint8_t* data, uint16_t len) {

    RingBuffer* rb[2] = {&_rb1, &_rb2};

    rb[!_index]->pushFromISR(reinterpret_cast<const uint8_t *>(data), len);
}
