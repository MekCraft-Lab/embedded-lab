/**
 *******************************************************************************
 * @file    server-intf.h
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
 * @date    2025/10/18
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

/* 1. OS */
#include "FreeRTOS.h"
#include "queue.h"

/* 2. standard lib */
#include <functional>
#include <string>


/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

enum class ServerError {
    NONE,
    UNKNOWN_FRAME_TYPE,
    SERVICE_BUSY,
};





/*-------- 3. interface ---------------------------------------------------------------------------------------------*/



template <typename FrameType, typename FrameData> class IServer {
  public:
    IServer() = default;
    using FrameHandler = std::function<void(FrameData&)>;
    using ErrorHandler = std::function<void(ServerError)>;

    void registerHandler(FrameType type, FrameHandler handler, UBaseType_t priority,
                                                        uint16_t stackSize, uint8_t queueLen) {
        TaskParam param;
        param.queue      = xQueueCreate(queueLen, sizeof(FrameData));
        param.handler    = handler;

        tasks_[type]     = param;

        std::string name = "Task_" + std::to_string((int)type);
        xTaskCreate(GenericTask, name.c_str(), stackSize, &tasks_[type], priority, nullptr);
    }


    void dispatch(FrameType type, FrameData& data) {

        auto it = tasks_.find(type);
        if (it != tasks_.end()) {
            if (xQueueSend(it->second.queue, &data, 0) == pdFALSE) {
                eh_(ServerError::SERVICE_BUSY);
            }
        } else if (eh_) {
            eh_(ServerError::UNKNOWN_FRAME_TYPE);
        }
    }

    void setErrorHandler(const ErrorHandler &handler){
        eh_ = handler;
    }

  private:
    struct TaskParam {
        QueueHandle_t queue;
        FrameHandler handler;
    };

    std::unordered_map<FrameType, TaskParam> tasks_;
    ErrorHandler eh_ = nullptr;

    [[noreturn]] static void GenericTask(void* pvParameters) {
        auto* param = static_cast<TaskParam *>(pvParameters);
        FrameData frame;

        for (;;) {
            if (xQueueReceive(param->queue, &frame, portMAX_DELAY) == pdPASS) {
                param->handler(frame);
            }
        }
    }
};