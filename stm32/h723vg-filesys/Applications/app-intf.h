/**
*******************************************************************************
* @file    app-intf.h
* @brief
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
* @date    2025/8/21
* @version 1.0
*******************************************************************************
*/

#pragma once



/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

/* I. OS */
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "queue.h"

/* II. standard lib */
#include <vector>

#include "stream_buffer.h"


/* ------- class prototypes-------------------------------------------------------------------------------------------*/

class IApplication {
  public:
    virtual ~IApplication() {};

    /**
     * @brief 初始化线程所需资源
     */
    virtual void init()                                          = 0;

    /**
     * @brief 线程主循环函数，由FreeRTOS Task调用
     */
    virtual void run()                                           = 0;


    /**
     * @brief 获取线程信息
     */
    [[nodiscard]] virtual const char* getName() const            = 0;
    [[nodiscard]] virtual float getRunTime() const               = 0;
    [[nodiscard]] virtual uint32_t getStackHighWaterMark() const = 0;
    [[nodiscard]] virtual TaskHandle_t getTaskHandle() const     = 0;

    virtual void initEvent()                                     = 0;
    virtual void waitInit()                                      = 0;

  protected:
    EventGroupHandle_t _initEvent = nullptr;

    class TaskInfo {
      public:
        TaskInfo(const char* name, uint16_t stackSize, IApplication* pThread, UBaseType_t priority,
                 StackType_t* stackBuf, StaticTask_t* pxTask)
            : name(name), stackSize(stackSize), pThread(pThread), priority(priority), stackBuf(stackBuf),
              pxTask(pxTask) {}
        const char* name       = nullptr;
        uint16_t stackSize     = 0;
        IApplication* pThread  = nullptr;
        UBaseType_t priority   = 0;
        TaskHandle_t tskHandle = nullptr;
        StackType_t* stackBuf  = nullptr;
        StaticTask_t* pxTask   = nullptr;
    };

    inline static std::vector<TaskInfo> _taskInfoRegistry;

    float _cpuUsage   = 0;
    float _stackUsage = 0;
    float _timeUsage  = 0;
};



class StaticAppBase : public IApplication {
  public:
    StaticAppBase(const char* name, uint16_t stackSize, UBaseType_t priority, StackType_t* stackBuf)
        : _taskInfo(name, stackSize, this, priority, stackBuf, &_staticTask) {

        /* event */
        _initEvent = xEventGroupCreateStatic(&_staticEventGroup);

        /* task */
        _registerThread(_taskInfo);
    }

    /**
     * @brief start all applications initialized.
     */
    static void startApplications() {
        for (auto eachApp : _taskInfoRegistry) {

            eachApp.tskHandle = xTaskCreateStatic(_taskEntry, eachApp.name, eachApp.stackSize, eachApp.pThread,
                                                  eachApp.priority, eachApp.stackBuf, eachApp.pxTask);
        }
    }

    /**
     * @brief set the initialization event
     */
    void initEvent() override { xEventGroupSetBits(_initEvent, 0x01); }

    /**
     * @brief waiting for the initialization event of application
     */
    void waitInit() override { xEventGroupWaitBits(_initEvent, 0x01, pdFALSE, pdFALSE, portMAX_DELAY); }



    /************* setter & getter **************/
    [[nodiscard]] const char* getName() const override { return _taskInfo.name; }

    [[nodiscard]] uint32_t getStackHighWaterMark() const override {
        return uxTaskGetStackHighWaterMark(_taskInfo.tskHandle);
    }

    [[nodiscard]] TaskHandle_t getTaskHandle() const override { return _taskInfo.tskHandle; }

    [[nodiscard]] float getRunTime() const override { return _runTime; }

  protected:
    TaskInfo _taskInfo;
    StaticEventGroup_t _staticEventGroup;
    StaticTask_t _staticTask;
    QueueHandle_t _queue;
    StaticQueue_t _staticQueue;
    float _runTime = 0;

  private:
    static void _registerThread(const TaskInfo& taskInfo) { _taskInfoRegistry.push_back(taskInfo); }

    [[noreturn]] static void _taskEntry(void* pvParameters) {
        auto* threadObj = static_cast<StaticAppBase*>(pvParameters);
        threadObj->init();
        threadObj->initEvent();
        while (true) {
            threadObj->run();
        }
    }
};



/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

extern StaticAppBase* _p_shell_thread;
extern StaticAppBase* pFileThread;
extern StaticAppBase* pConsoleThread;


/* ------- function implement ----------------------------------------------------------------------------------------*/
