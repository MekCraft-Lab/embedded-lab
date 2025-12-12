/**
 *******************************************************************************
 * @file    app-dispaly.h
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

#ifndef PLAY_STATION_APP_DISPALY_H
#define PLAY_STATION_APP_DISPALY_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "application-base.h"

/* II. OS */
#include "freertos/semphr.h"

/* III. middlewares */

#include "esp_lvgl_port.h"



/* IV. drivers */
#include "bitmap.h"
#include "lcd-i80.h"

/* V. standard lib */



#define DISP_WIDTH  240
#define DISP_HEIGHT 240



/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class DisplayApp final : public StaticAppBase {
  public:
    DisplayApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    uint8_t rxMsg(void *msg, uint16_t size, TickType_t timeout)  override;
    uint8_t checkout();

    /************ setter & getter ***********/
    static DisplayApp& instance();


  private:
    /* message interface */
    
    // 1. message queue
    QueueHandle_t queue;
    
    // 2. mutex
    
    // 3. semphr
    SemaphoreHandle_t _nofrendoUpdate;

    // 4. notify
    
    // 5. stream or message
    
    // 6. event group

    friend void playSound(void* buf, int size);

    friend int vidInit(int width, int height);

    friend void vidShutdown();

    friend int vidSetMode(int width, int height);

    friend void vidSetPalette(rgb_t* palette);

    friend void vidClear(uint8 color);

    friend bitmap_t* vidLockWrite();

    friend void vidFreeWrite(int num_dirties, rect_t* dirty_rects);

    friend void vidBlit(bitmap_t* primary, int num_dirties, rect_t* dirty_rects);

};
#endif


#ifdef __cplusplus
extern "C" {
#endif

    /* C Interface */

int checkoutFromLVGL(int width, int height);

#ifdef __cplusplus
}
#endif


void playSound(void* buf, int size);

int vidInit(int width, int height);

void vidShutdown();

int vidSetMode(int width, int height);

void vidSetPalette(rgb_t* palette);

void vidClear(uint8 color);

bitmap_t* vidLockWrite();

void vidFreeWrite(int num_dirties, rect_t* dirty_rects);

void vidBlit(bitmap_t* primary, int num_dirties, rect_t* dirty_rects);

int logout(const char* msg);


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif