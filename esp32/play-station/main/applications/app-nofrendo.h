/**
 *******************************************************************************
 * @file    app-nofrendo.h
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
 * @date    2025/12/12
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef PLAY_STATION_APP_NOFRENDO_H
#define PLAY_STATION_APP_NOFRENDO_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "application-base.h"

/* II. OS */
#include "freertos/semphr.h"


/* III. middlewares */


/* IV. drivers */
#include "osd.h"
#include "noftypes.h"


/* V. standard lib */




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class NofrendoApp final : public StaticAppBase {
  public:
    NofrendoApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    uint8_t rxMsg(void* msg, uint16_t size, TickType_t timeout) override;

    /************ setter & getter ***********/
    static NofrendoApp& instance();


  private:
    /* message interface */

    // 1. message queue

    // 2. mutex

    // 3. semphr

    // 4. notify

    // 5. stream or message

    // 6. event group


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