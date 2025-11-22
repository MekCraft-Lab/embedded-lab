/**
 *******************************************************************************
 * @file    ringBuffer.cpp
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


/* ------- define --------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "ringBuffer.h"

#include <future>

#include "event_groups.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/


/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/


/* ------- function implement ----------------------------------------------------------------------------------------*/

RingBuffer::RingBuffer(uint8_t* buffer, const size_t capacity,QueueHandle_t* queue)
    : _buffer(buffer), _capacity(capacity), _pQueue(queue){

    _mutex = xSemaphoreCreateMutex();
    _waitForReceive = xSemaphoreCreateBinary();

}

bool RingBuffer::push(const uint8_t* data, const size_t len, TickType_t timeout) {
    if (xSemaphoreTake(_mutex, timeout) != pdPASS) {
        return false;
    };

    size_t offset;

    uint32_t startTime = xTaskGetTickCount();
    uint8_t giveFlag = 0;
    while (_size + len > _capacity || !uxQueueSpacesAvailable(*_pQueue)) {
        // the buffer or queue is full, release the lock and wait for the receiving.
        xSemaphoreGive(_mutex);
        giveFlag = 1;

        // unable to wait for the receiving then return.
        if (xSemaphoreTake(_waitForReceive, timeout) != pdPASS) {
            return false;
        };
        // timeout then return.
        if (xTaskGetTickCount() - startTime > timeout) {
            return false;
        }
    }

    // give the lock then take it again
    if (giveFlag) {
        if (xSemaphoreTake(_mutex, timeout) != pdPASS) {
            return false;
        }
    }

    // now the buffer and the queue are all available, so start the option.
    offset = _head;

    for (size_t i = 0; i < len; ++i) {
        _buffer[_head] = data[i];
        _head = (_head + 1) % _capacity;
        _size++;
    }

    BufferMsg msg(offset, len);

    auto    res = xQueueSend(*_pQueue, &msg, portMAX_DELAY);

    xSemaphoreGive(_mutex);
    return res == pdPASS;
}

bool RingBuffer::pushFromISR(const uint8_t* data, const size_t len) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xSemaphoreTakeFromISR(_mutex, &xHigherPriorityTaskWoken) != pdPASS) {
        return false;
    };

    size_t offset;


    uint8_t giveFlag = 0;

    if (_size + len > _capacity) {
        xSemaphoreGiveFromISR(_mutex, &xHigherPriorityTaskWoken);
        return false;
    }


    // now the buffer and the queue are all available, so start the option.

    for (size_t i = 0; i < len; ++i) {
        _buffer[_head] = data[i];
        _head = (_head + 1) % _capacity;
        _size++;
    }

    BufferMsg msg(offset, len);

    auto    res = xQueueSendFromISR(*_pQueue, &msg,&xHigherPriorityTaskWoken);

    xSemaphoreGiveFromISR(_mutex, &xHigherPriorityTaskWoken);
    return res == pdPASS;
}

bool RingBuffer::pop(uint8_t* data, size_t& len, TickType_t timeout) {
    if (xSemaphoreTake(_mutex, timeout) != pdPASS) {
        return false;
    }

    BufferMsg msg;
    if (xQueueReceive(*_pQueue, &msg, timeout) != pdPASS) {
        xSemaphoreGive(_mutex);
        return false;
    }

    for (size_t i = 0; i < msg.len; ++i) {
        data[i] = _buffer[msg.offset + i];
        _tail = (_tail + 1) % _capacity;
        _size--;
        len = i;
    }
    xSemaphoreGive(_waitForReceive);;
    xSemaphoreGive(_mutex);

    return true;
}

uint8_t RingBuffer::getData() {
    uint8_t data = _buffer[_tail];
    _tail = (_tail + 1) % _capacity;
    _size--;
    return data;
}

void RingBuffer::tailIncrease(uint16_t num) {
    _tail = (_tail + num) % _capacity;
    _size -= num;
    xSemaphoreGive(_waitForReceive);
}
