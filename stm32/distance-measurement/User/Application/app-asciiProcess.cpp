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

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "AsciiProcess"

#define APPLICATION_STACK_SIZE 512

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

static AsciiProcessApp asciProcessApp;




/* ------- message interface attribute -------------------------------------------------------------------------------*/






/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


AsciiProcessApp::AsciiProcessApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 0, nullptr){
}


AsciiProcessApp& AsciiProcessApp::instance() {
    return asciProcessApp;
}


void AsciiProcessApp::init() {
    /* driver object initialize */
}


void AsciiProcessApp::run() {
 
}



uint8_t AsciiProcessApp::rxMsg(void* msg, uint16_t size) {

    return 0;
}

uint8_t AsciiProcessApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) {

    return 0;
}



