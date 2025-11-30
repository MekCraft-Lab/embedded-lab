/**
 *******************************************************************************
 * @file    app-graphic.h
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
 * @date    2025/11/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef __APP_GRAPH_H__
#define __APP_GRAPH_H__




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

/* I. interface */
#include "app-intf.h"

/* II. OS */

#include "semphr.h"
#include "stream_buffer.h"

/* III. middlewares */

/* IV. drivers */
#include "gpio.h"
#include "fmc.h"
#include "../../Drivers/Devices/ILI9481/drv-ili9481.h"

/* V. standard lib */




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class GraphicApp final : public StaticAppBase {
  public:
    GraphicApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    uint8_t rxMsg(void *msg, uint16_t size, TickType_t timeout)  override;

    /************ setter & getter ***********/
    static GraphicApp& instance();

    static void delayMs(uint32_t ms);


  private:
    /* message interface */
    uint8_t _index{};
    StaticStreamBuffer_t _stm{};
    StreamBufferHandle_t _sbHandle;

};





/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif