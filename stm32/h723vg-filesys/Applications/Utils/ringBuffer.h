/**
 *******************************************************************************
 * @file    ringbuffer.h
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
 * @date    2025/8/28
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/


#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/





/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class BufferMsg {
public:
    BufferMsg(){}
    BufferMsg(uint16_t offset, uint16_t len): offset(offset), len(len) {}
    uint16_t offset;
    uint16_t len;
};


class RingBuffer {
public:

    RingBuffer(uint8_t* buffer, size_t capacity, QueueHandle_t* queue);
    bool push(const uint8_t *data, size_t len, TickType_t timeout); // 写入数据并发送队列
    bool pushFromISR(const uint8_t *data, size_t len);

    bool pop(uint8_t *data, size_t &len, TickType_t timeout);               // 从队列获取下一个消息



    [[nodiscard]] SemaphoreHandle_t getMutex() const{return _mutex;}
    [[nodiscard]] SemaphoreHandle_t getBinary() const{return _waitForReceive;}
    [[nodiscard]] uint16_t getSize() const {return _size;}
    [[nodiscard]] uint16_t getCapacity() const {return _capacity;}
    [[nodiscard]] uint8_t* getBase() const {return _buffer;}
    uint8_t getData();
    void tailIncrease(uint16_t num);

private:
    uint8_t* _buffer;
    size_t _capacity;
    size_t _head{};
    size_t _tail{};
    size_t _size{};

    SemaphoreHandle_t _mutex;
    QueueHandle_t* _pQueue;
    SemaphoreHandle_t _waitForReceive;
};

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/


/*-------- 5. factories ----------------------------------------------------------------------------------------------*/