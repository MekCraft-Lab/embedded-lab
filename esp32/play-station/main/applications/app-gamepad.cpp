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
#define UART_RX_PIN    12
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

#define DISPATCH_BTN(cur, last, evt)                                                                                   \
    do {                                                                                                               \
        if ((cur) != (last)) {                                                                                         \
            event_get(evt)((cur) ? 1 : 0);                                                  \
            (last) = (cur);                                                                                            \
        }                                                                                                              \
    } while (0)




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
            uart_read_bytes(UART_NUM, gamepad.getBufferPointer(), event.size >= 16 ? 16 : event.size,
                            pdMS_TO_TICKS(100));
            static uint8_t i;
        }
    }
}



uint8_t GamepadApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t GamepadApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }

extern uint8_t checkFlag;
extern "C" void osd_getinput(void) {
    uint8_t A, B, RB, LB, Up, Down, Left, Right;
    A     = GamepadApp::instance().gamepad.getA();
    B     = GamepadApp::instance().gamepad.getB();
    RB    = GamepadApp::instance().gamepad.getRB();
    LB    = GamepadApp::instance().gamepad.getLB();
    Up    = GamepadApp::instance().gamepad.getUp();
    Down  = GamepadApp::instance().gamepad.getDown();
    Left  = GamepadApp::instance().gamepad.getLeft();
    Right = GamepadApp::instance().gamepad.getRight();

    /* 保存上一次状态（static，零内存抖动） */
    static uint8_t lastA, lastB, lastRB, lastLB;
    static uint8_t lastUp, lastDown, lastLeft, lastRight;

    /* 只在状态变化时发送事件 */
    DISPATCH_BTN(A,     lastA,     44); // A
    DISPATCH_BTN(B,     lastB,     45); // B
    DISPATCH_BTN(RB,    lastRB,    46); // Start
    DISPATCH_BTN(LB,    lastLB,    47); // Select
    DISPATCH_BTN(Up,    lastUp,    48);
    DISPATCH_BTN(Down,  lastDown,  49);
    DISPATCH_BTN(Left,  lastLeft,  50);
    DISPATCH_BTN(Right, lastRight, 51);

}