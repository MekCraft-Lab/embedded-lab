/**
 *******************************************************************************
 * @file    app-wirelessComm.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef H723VG_MAIN_CTRL_APP_WIRELESSCOMM_H
#define H723VG_MAIN_CTRL_APP_WIRELESSCOMM_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "application-base.h"

/* II. OS */

#include "semphr.h"

/* III. middlewares */


/* IV. drivers */


/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);


class WirelessCommApp final : public StaticAppBase {
  public:
    WirelessCommApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    uint8_t rxMsg(void* msg, uint16_t size, TickType_t timeout) override;

    /************ setter & getter ***********/
    static WirelessCommApp& instance();


  private:
    /* message interface */

    // 1. message queue

    // 2. mutex

    // 3. semphr
    xSemaphoreHandle _coProcessorReady;

    // 4. notify

    // 5. stream or message

    // 6. event group

    friend void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
};
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* C Interface */

#ifdef __cplusplus
}
#endif




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif