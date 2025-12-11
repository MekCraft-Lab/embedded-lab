/**
 *******************************************************************************
 * @file    app-gamepad.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef PLAY_STATION_APP_GAMEPAD_H_H
#define PLAY_STATION_APP_GAMEPAD_H_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "application-base.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */

#include "Gamepad.h"

/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class GamepadApp :public StaticAppBase {
  public:
    GamepadApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    uint8_t rxMsg(void *msg, uint16_t size, TickType_t timeout)  override;

    /************ setter & getter ***********/
    static GamepadApp& instance();


  private:

    Gamepad _gamepad;

    /* message interface */
    
    // 1. message queue
    QueueHandle_t _rxQueue;
    
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