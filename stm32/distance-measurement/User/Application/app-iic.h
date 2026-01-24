/**
 *******************************************************************************
 * @file    app-iic.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef DISTANCE_MEASUREMENT_APP_IIC_H
#define DISTANCE_MEASUREMENT_APP_IIC_H


/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#ifdef __cplusplus

/* I. interface */

#include "application-base.h"
#include "semphr.h"

/* II. OS */


/* III. middlewares */


/* IV. drivers */
#include  "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"




/* V. standard lib */





/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class IicApp final : public StaticAppBase {
  public:
    IicApp();

    void init() override;

    void run() override;

    uint8_t rxMsg(void* msg, uint16_t size = 0) override;

    uint8_t rxMsg(void *msg, uint16_t size, TickType_t timeout)  override;

    /************ setter & getter ***********/
    static IicApp& instance();


  private:
    /* message interface */
    
    // 1. message queue
    
    // 2. mutex
    
    // 3. semphr
    xSemaphoreHandle _txCpltSemaphore;
    // 4. notify
    
    // 5. stream or message
    
    // 6. event group
    friend void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c);
    friend void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c);
};
#endif


#ifdef __cplusplus
extern "C" {
#endif

    /* C Interface */
    void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c);
    void HAL_I2C_ErrorCallback(I2C_HandleTypeDef* hi2c) ;
#ifdef __cplusplus
}
#endif




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

#endif