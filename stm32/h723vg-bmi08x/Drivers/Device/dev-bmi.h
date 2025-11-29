/**
 *******************************************************************************
 * @file    dev-bmi.h
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
 * @date    2025/11/26
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef H723VG_BMI08X_DEV_BMI_H
#define H723VG_BMI08X_DEV_BMI_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "gpio.h"
#include "spi.h"
#include "dev-bmi-def.h"

#include <functional>
#include <vector>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#define BMI_DMA_BUF_LEN 64




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

enum class BmiErr {
    NONE,
    SUCCESS,
    PARAM_ERROR,
    ADDR_NULL,
    ERROR_IN_HAL,
    COMM_TIMEOUT,
    ERROR,
};

class BmiRxOpt {

  public:
    BmiRxOpt(void* pData, const uint16_t len, const uint16_t offset) : _pData(pData), _len(len), _offset(offset) {};

    /*********** setter & getter ***********/
    [[nodiscard]] void* getPData() const { return _pData; }
    [[nodiscard]] uint16_t getLen() const { return _len; }
    [[nodiscard]] uint16_t getOffset() const { return _offset; }

  private:
    void* _pData     = nullptr;
    uint16_t _len    = 0;
    uint16_t _offset = 0;
};

enum class Bmi088Chip {
    NONE,
    ACC,
    GYRO,
};


class Bmi088 {
  public:
    using CommCompletion = std::function<bool(uint32_t timeout)>;

    Bmi088(SPI_HandleTypeDef* handle, void* pDmaTxBuf, void* pDmaRxBuf, CommCompletion waitForTransmit, CommCompletion waitForReceive,
           CommCompletion waitForTransmitReceive, GPIO_TypeDef* ps1Port, uint16_t ps1Pin, GPIO_TypeDef* ps2Port,
           uint16_t ps2Pin);

    BmiErr init();
    BmiErr readRegister(Bmi088Chip chip, uint8_t regAddr, uint16_t len, uint32_t timeout);
    BmiErr readRegister(Bmi088Chip chip, uint8_t regAddr, void* pData, uint16_t len, uint32_t timeout);


    BmiErr writeRegister(Bmi088Chip chip, uint8_t, uint16_t, uint16_t timeout);
    BmiErr writeRegister(Bmi088Chip chip, uint8_t regAddr, void* pData, uint16_t len, uint16_t timeout);


    BmiErr accSelfCheck();

    BmiErr readAccIDAsync();
    BmiErr readGyroIDAsync();

    BmiErr updateGyroAndAccSync(uint16_t* avX, uint16_t* avY, uint16_t* avZ, uint16_t* aX, uint16_t* aY, uint16_t* aZ,
                                uint32_t timeout);
    BmiErr updateGyroAndAccSync(void* pData, uint32_t timeout);



    BmiErr asyncRxCallback();
    BmiErr asyncTxCallback();



    /* setter & getter */

    uint8_t* getDmaTxBuf() { return _dmaTxBuf; }

    uint8_t* getDmaRxBuf() { return _dmaRxBuf; }

    struct __attribute__((packed)) AngularVelocity {
        int16_t x;
        int16_t y;
        int16_t z;
    } _angularVelocity;

    struct __attribute__((packed)) Acceleration {
        int16_t x;
        int16_t y;
        int16_t z;
    } _acceleration;


  private:
    SPI_HandleTypeDef* _handle             = nullptr;
    CommCompletion _waitForTransmit        = nullptr;
    CommCompletion _waitForReceive         = nullptr;
    CommCompletion _waitForTransmitReceive = nullptr;
    uint16_t _accChipID                    = 0x00;
    uint16_t _gyroChipID                   = 0x00;
    uint8_t* _dmaTxBuf = nullptr;
    uint8_t* _dmaRxBuf = nullptr;
    size_t _bufLen = 0;
    GPIO_TypeDef* _ps1Port;
    GPIO_TypeDef* _ps2Port;
    GPIO_TypeDef* _psCurrentPort;
    uint16_t _ps1Pin;
    uint16_t _ps2Pin;
    uint16_t _psCurrentPin;


    std::pmr::vector<BmiRxOpt> _opts;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*H723VG_BMI08X_DEV_BMI_H*/
