/**
 *******************************************************************************
 * @file    dev-bmi.cpp
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


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "dev-bmi.h"

#include "dev-bmi-def.h"

#include <cstring>




/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/

#define __BMI_DELAY_MS(x)                                                                                              \
    do {                                                                                                               \
        uint32_t currentTimestamp = TIM24->CNT;                                                                        \
        while (TIM24->CNT - currentTimestamp < x * 1000)                                                               \
            ;                                                                                                          \
    } while (0)




/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


static void memcpyReverseOrder(void* dest, const void* src, size_t len) {
    auto d           = static_cast<uint8_t*>(dest);
    const uint8_t* s = static_cast<const uint8_t*>(src) + len - 1;

    while (len--) {
        *d++ = *s--;
    }
}


Bmi088::Bmi088(SPI_HandleTypeDef* handle, void* pDmaTxBuf, void* pDmaRxBuf, CommCompletion transmit,
               CommCompletion receive, CommCompletion transmitReceive, GPIO_TypeDef* ps1Port, uint16_t ps1Pin,
               GPIO_TypeDef* ps2Port, uint16_t ps2Pin)
    : _handle(handle), _waitForTransmit(transmit), _waitForReceive(receive), _waitForTransmitReceive(transmitReceive),
      _ps1Port(ps1Port), _ps1Pin(ps1Pin), _ps2Port(ps2Port), _ps2Pin(ps2Pin),
      _dmaTxBuf(static_cast<uint8_t*>(pDmaTxBuf)), _dmaRxBuf(static_cast<uint8_t*>(pDmaRxBuf)) {
    if (handle == nullptr) {
        while (1)
            ;
    }
}

BmiErr Bmi088::init() {

    // 通过一次读取加速度计ID的操作让PS1产生一次上升沿，将加速度计通信模式改为SPI

    return readAccIDAsync();
}

/**
 * @brief 读取寄存器
 * @param chip 设备芯片，陀螺仪或加速度计\
 * @param regAddr 寄存器地址
 * @param len 数据长度
 * @param timeout 超时时间，设置为零则为异步读取
 * @return 错误码
 */
BmiErr Bmi088::readRegister(Bmi088Chip chip, uint8_t regAddr, uint16_t len, uint32_t timeout) {

    BmiErr err = BmiErr::SUCCESS;


    switch (chip) {
        case Bmi088Chip::ACC: {
            _psCurrentPort = _ps1Port;
            _psCurrentPin  = _ps1Pin;
        } break;

        case Bmi088Chip::GYRO: {
            _psCurrentPort = _ps2Port;
            _psCurrentPin  = _ps2Pin;
        } break;

        default: {
            return BmiErr::PARAM_ERROR;
        }
    }


    if (len == 0) {
        return BmiErr::PARAM_ERROR;
    }

    _dmaTxBuf[0]         = regAddr | 0x80;

    _psCurrentPort->BSRR = _psCurrentPin << 16;

    if (HAL_SPI_TransmitReceive_DMA(_handle, _dmaTxBuf, _dmaRxBuf, len) != HAL_OK) {
        return BmiErr::ERROR_IN_HAL;
    }

    if (timeout) {



        if (!_waitForTransmitReceive(timeout)) {
            return BmiErr::COMM_TIMEOUT;

        }
        _psCurrentPort->BSRR = _psCurrentPin;


    }


    return err;
}


/**
 * @brief 寄存器写入, 用于驱动外的额外访问
 * @param chip 芯片片选
 * @param regAddr 寄存器地址
 * @param pData 数据地址
 * @param len 发送长度
 * @param timeout 超时，如果为零则是异步读取
 * @return 错误码
 */
BmiErr Bmi088::readRegister(Bmi088Chip chip, uint8_t regAddr, void* pData, uint16_t len, uint32_t timeout) {

    BmiErr err = BmiErr::SUCCESS;


    switch (chip) {
        case Bmi088Chip::ACC: {
            _psCurrentPort = _ps1Port;
            _psCurrentPin  = _ps1Pin;
        } break;

        case Bmi088Chip::GYRO: {
            _psCurrentPort = _ps2Port;
            _psCurrentPin  = _ps2Pin;
        } break;

        default: {
            return BmiErr::PARAM_ERROR;
        }
    }


    if (len == 0) {
        return BmiErr::PARAM_ERROR;
    }


    _dmaTxBuf[0]         = regAddr | 0x80;

    _psCurrentPort->BSRR = _psCurrentPin << 16;

    if (HAL_SPI_TransmitReceive_DMA(_handle, _dmaTxBuf, _dmaRxBuf, len) != HAL_OK) {
        return BmiErr::ERROR_IN_HAL;
    }

    if (timeout) {


        if (!_waitForTransmitReceive(timeout)) {
            return BmiErr::COMM_TIMEOUT;
        }

        memcpy(pData, _dmaRxBuf, len);

        return err;

    }

    const BmiRxOpt id(pData, len,  1);

    _opts.push_back(id);


    return err;
}


/**
 * @brief 写入寄存器
 * @param chip 芯片片选
 * @param regAddr 寄存器地址
 * @param pData 数据地址
 * @param len 发送长度
 * @param timeout 超时时间，如果为零为异步
 * @return 错误码
 */
BmiErr Bmi088::writeRegister(Bmi088Chip chip, uint8_t regAddr, void* pData, uint16_t len, uint16_t timeout) {
    BmiErr err = BmiErr::SUCCESS;
    uint8_t* buf = reinterpret_cast<uint8_t*>(pData);


    switch (chip) {
        case Bmi088Chip::ACC: {
            _psCurrentPort = _ps1Port;
            _psCurrentPin  = _ps1Pin;
        } break;

        case Bmi088Chip::GYRO: {
            _psCurrentPort = _ps2Port;
            _psCurrentPin  = _ps2Pin;
        } break;

        default: {
            return BmiErr::PARAM_ERROR;
        }
    }


    if (len == 0) {
        return BmiErr::PARAM_ERROR;
    }

    _dmaTxBuf[0]         = regAddr;
    memcpy(&_dmaTxBuf[1], buf, len);


    _psCurrentPort->BSRR = _psCurrentPin << 16;



    if (HAL_SPI_Transmit_DMA(_handle, _dmaTxBuf, len + 1) != HAL_OK) {
        return BmiErr::ERROR_IN_HAL;
    };

    if (timeout) {

        if (!_waitForTransmit(timeout)) {
            return BmiErr::COMM_TIMEOUT;
        }

    }

    // 超时错误


    return err;
}

/**
 * @brief 加速度计自检
 * @attention 未启用
 * @return
 */
BmiErr Bmi088::accSelfCheck() {

    _ps1Port->BSRR  = _psCurrentPin << 16;
    _ps2Port->BSRR  = _psCurrentPin;

    uint8_t txbuf[] = {0x41, 0x03, 0x40, 0xA7};
    HAL_SPI_Transmit(_handle, txbuf, sizeof(txbuf), 0xFFFF);
    _ps1Port->BSRR = _psCurrentPin;
    __BMI_DELAY_MS(8);

    txbuf[0] = 0x6D;
    txbuf[1] = 0x0D;
    HAL_SPI_Transmit(_handle, txbuf, 2, 0xFFFF);
    _ps1Port->BSRR = _psCurrentPin;
    __BMI_DELAY_MS(80);

    uint16_t posX, posY, posZ, negX, negY, negZ;

    for (uint8_t i = 0; i < 3; i++) {
        txbuf[0] = static_cast<uint8_t>(Bmi088AccRegister::ACC_DATA) + i;
        HAL_SPI_Transmit(_handle, txbuf, 1, 0xFFFF);
    }

    /* 不想写了 */

    return BmiErr::SUCCESS;
}

/**
 * @brief 异步读取加速度计ID
 * @return
 */
BmiErr Bmi088::readAccIDAsync() {

    BmiErr err = BmiErr::SUCCESS;

    err        = readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_CHIP_ID, 3, 0);

    if (err != BmiErr::SUCCESS) {
        return err;
    }

    const BmiRxOpt id(&_accChipID, 1, 2);

    _opts.push_back(id);

    return err;
}


/**
 * @brief 异步读取陀螺仪数据
 * @return 错误码
 */
BmiErr Bmi088::readGyroIDAsync() {
    BmiErr err = BmiErr::SUCCESS;

    err        = readRegister(Bmi088Chip::GYRO, (uint8_t)Bmi088GyroRegister::GYRO_CHIP_ID, 2, 0);

    if (err != BmiErr::SUCCESS) {
        return err;
    }

    const BmiRxOpt id(&_gyroChipID, 1, 1);

    _opts.push_back(id);

    return err;
}


/**
 * @brief 同步读取六轴数据
 * @param avX X轴角速度变量地址
 * @param avY Y轴角速度变量地址
 * @param avZ Z轴角速度变量地址
 * @param aX X轴加速度变量地址
 * @param aY Y轴加速度变量地址
 * @param aZ Z轴加速度变量地址
 * @param timeout 超时时间，不得为零
 * @return 错误码
 */
BmiErr Bmi088::updateGyroAndAccSync(uint16_t* avX, uint16_t* avY, uint16_t* avZ, uint16_t* aX, uint16_t* aY,
                                    uint16_t* aZ, uint32_t timeout) {
    BmiErr err = BmiErr::SUCCESS;

    if (timeout == 0) {
        return BmiErr::PARAM_ERROR;
    }

    uint8_t temp[12];
    for (uint8_t i = 0; i < 6; i += 2) {
        err = readRegister(Bmi088Chip::GYRO, (uint8_t)Bmi088GyroRegister::RATE_DATA + i, 2, timeout);
        if (err != BmiErr::SUCCESS) {
            return err;
        }

        temp[i] = _dmaRxBuf[0];

        err     = readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_DATA + i, 2, timeout);
        if (err != BmiErr::SUCCESS) {
            return err;
        }

        temp[i + 6] = _dmaRxBuf[0];
    }

    *avX               = temp[0] | temp[1] << 8;

    _angularVelocity.x = *avX;

    *avY               = temp[2] | temp[3] << 8;

    _angularVelocity.y = *avY;

    *avZ               = temp[4] | temp[5] << 8;

    _angularVelocity.z = *avZ;

    *aX                = temp[6] | temp[7] << 8;

    _acceleration.x    = *aX;

    *aY                = temp[8] | temp[9] << 8;

    _acceleration.y    = *aY;

    *aZ                = temp[10] | temp[11] << 8;

    _acceleration.z    = *aZ;

    return err;
}


BmiErr Bmi088::updateGyroAndAccSync(void* pData, uint32_t timeout) {
    BmiErr err = BmiErr::SUCCESS;

    if (timeout == 0) {
        return BmiErr::PARAM_ERROR;
    }

    if (pData == nullptr) {
        return BmiErr::ADDR_NULL;
    }

    uint8_t* temp = reinterpret_cast<uint8_t*>(pData);


    err = readRegister(Bmi088Chip::GYRO, (uint8_t)Bmi088GyroRegister::RATE_DATA, 7, timeout);


    if ( err != BmiErr::SUCCESS ) {
        return err;
    }
    memcpy(temp, _dmaRxBuf + 1, 6);


    err = readRegister(Bmi088Chip::ACC, (uint8_t)Bmi088AccRegister::ACC_DATA, 8, timeout);

    if (err != BmiErr::SUCCESS) {
        return err;
    }

    memcpy(temp + 6, _dmaRxBuf + 2, 6);

    return err;
}




/**
 * @brief 异步读取回调
 * @return
 */
BmiErr Bmi088::asyncRxCallback() {
    BmiErr err           = BmiErr::SUCCESS;

    _psCurrentPort->BSRR = _psCurrentPin;

    uint8_t index        = 0;
    uint16_t cnt = 0;
    for (auto& eachOpt : _opts) {
        memcpyReverseOrder(eachOpt.getPData(), index + _dmaRxBuf + eachOpt.getOffset(), eachOpt.getLen());
        index += eachOpt.getLen();

        if (cnt++ >= BMI_DMA_BUF_LEN) {
            return BmiErr::PARAM_ERROR;
        }
    }

    _opts.clear();

    return err;
}

/**
 * @brief 异步写入回调
 * @return
 */
BmiErr Bmi088::asyncTxCallback() {
    _psCurrentPort->BSRR = _psCurrentPin;

    return BmiErr::SUCCESS;
}