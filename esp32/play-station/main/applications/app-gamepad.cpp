/**
 *******************************************************************************
 * @file    app-gamepad.cpp
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
 * @date    2025/12/11
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/

#define UART_NUM       UART_NUM_1
#define UART_RX_PIN    22
#define UART_BAUD_RATE 921600
#define BUF_SIZE       16




/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-gamepad.h"

/* II. other application */

#include "driver/uart.h"
#include "esp_log.h"

/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "GamepadApp"

#define APPLICATION_STACK_SIZE 2048

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];



/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


GamepadApp::GamepadApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


GamepadApp& GamepadApp::instance() {
    static GamepadApp instance;
    return instance;
}


void GamepadApp::init() {
    /* driver object initialize */

    ESP_LOGI(APPLICATION_NAME, "Initializing GamepadApp");

    uart_config_t uart_cfg = {.baud_rate  = UART_BAUD_RATE,
                              .data_bits  = UART_DATA_8_BITS,
                              .parity     = UART_PARITY_DISABLE,
                              .stop_bits  = UART_STOP_BITS_1,
                              .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
                              .source_clk = UART_SCLK_DEFAULT};

    uart_param_config(UART_NUM, &uart_cfg);
    uart_set_pin(UART_NUM, -1, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // 安装驱动
    uart_driver_install(UART_NUM, 192, 192, 5, &_rxQueue, 0);

    // FIFO 阈值和超时
    uart_set_rx_full_threshold(UART_NUM, 32); // FIFO ≥ 32 字节触发
    uart_set_rx_timeout(UART_NUM, 1);         // 超时触发

    // 使能 RX 中断
    uart_enable_rx_intr(UART_NUM);
}


void GamepadApp::run() {

    uart_event_t event;
    if (xQueueReceive(_rxQueue, &event, APPLICATION_PRIORITY)) {

        if (event.type == UART_DATA) {
            // event.size 指示 RX buffer 当前可读字节数
            uart_read_bytes(UART_NUM, _gamepad.getBufferPointer(), event.size >= 16 ? 16 : event.size,
                                      pdMS_TO_TICKS(100));
        }
    }
}



uint8_t GamepadApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t GamepadApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }
