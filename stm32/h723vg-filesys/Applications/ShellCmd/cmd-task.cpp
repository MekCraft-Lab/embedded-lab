/**
 *******************************************************************************
 * @file    task.cpp
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
 * @date    2025/8/26
 * @version 1.0
 *******************************************************************************
 */


/* ------- define --------------------------------------------------------------------------------------------------*/


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "cmd-task.h"

#include <cstring>


/* ------- class prototypes-----------------------------------------------------------------------------------------*/


/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/


/* ------- function implement ----------------------------------------------------------------------------------------*/

int ps(int argc, char* argv[]) {
    char statsBuf[512];

    vTaskList(statsBuf);
    logPrintln("\n===================================================\r\n%s\r\n", statsBuf);
    return 114514;
}

int top(int argc, char* argv[]) {
    char statsBuf[512];
    vTaskGetRunTimeStats(statsBuf);
    logPrintln("\n===================================================\r\n%s\r\n", statsBuf);
    return 114514;
}

int osFree(int argc, char* argv[]) { return xPortGetFreeHeapSize(); }

int stack(int argc, char* argv[]) {
    if (argc < 2) {
        logPrintln("ERROR: Too few arguments");
        return -1;
    }


    TaskStatus_t* taskStatusArray;
    volatile UBaseType_t arraySize, x;
    UBaseType_t highWaterMark = 0xFFFF; // 默认返回大值表示没找到

    // 获取任务总数
    arraySize                 = uxTaskGetNumberOfTasks();

    // 分配数组存储任务状态
    taskStatusArray           = static_cast<TaskStatus_t*>(pvPortMalloc(arraySize * sizeof(TaskStatus_t)));
    if (taskStatusArray == NULL) {
        logPrintln("ERROR: Unable to allocate memory for task status array");
    }

    // 获取系统所有任务状态
    arraySize = uxTaskGetSystemState(taskStatusArray, arraySize, NULL);

    for (x = 0; x < arraySize; x++) {

        if (std::strcmp(taskStatusArray[x].pcTaskName, argv[1]) == 0) {
            // 找到任务，调用 uxTaskGetStackHighWaterMark
            highWaterMark = uxTaskGetStackHighWaterMark(taskStatusArray[x].xHandle);
            break;
        }
    }

    if (highWaterMark == 0xFFFF) {
        logPrintln("ERROR: No such task");
    }

    vPortFree(taskStatusArray);
    return highWaterMark;
}