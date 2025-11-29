/**
 *******************************************************************************
 * @file    app-console.h
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
 * @date    2025/8/26
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef __APP_HOST_H__
#define __APP_HOST_H__




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/
#ifdef __cplusplus

/* I. interface */
#include "application-base.h"

/* II. OS */

#include "semphr.h"
#include "stream_buffer.h"

/* III. middlewares */

/* IV. drivers */
#include "usart.h"

/* V. standard lib */
#include <cstring>




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class HostApp final : public StaticAppBase {
  public:
    HostApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    bool println(const char* str, ...);

    bool println(uint8_t ISR, const char *fmt, ...);

    uint8_t rxMsg(void *msg, uint16_t size, TickType_t timeout)  override;

    /************ setter & getter ***********/
    static HostApp& instance();


  private:
    /* message interface */
    uint8_t _index;
    StaticStreamBuffer_t _stm;
    StreamBufferHandle_t _sbHandle;
    xSemaphoreHandle _waitForTransmitLock;

    friend void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
};
#endif


#ifdef __cplusplus
extern "C" {
#endif
    void hostPrintln(const char* fmt, ...);
#ifdef __cplusplus
}
#endif




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif