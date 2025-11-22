/**
 *******************************************************************************
 * @file    app-console.h
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @notes
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

/* I. interface */
#include "app-intf.h"

/* II. OS */
#include "queue.h"

/* III. middlewares */
#include "Utils/ringBuffer.h"

/* IV. drivers */
#include "../../Drivers/Communications/comm-intf.h"




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/





/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

/* 1. thread interface */
class ConsoleApp final : public StaticAppBase {
  public:
    ConsoleApp();

    void init() override;
    void run() override;

    /**
     * @brief
     * @param fmt format
     * @param ... var
     */
    void println(const char* fmt, ...);
    void error(const char* fmt, ...);
    void output(void* data, uint16_t len);

    /**
     * @brief get the instance of application
     * @return instance of application
     */
    static ConsoleApp& instance();

  private:
    IComm* _comm{};
    xQueueHandle _logQueue{};
    RingBuffer _rb1;
    RingBuffer _rb2;
    uint8_t _index{};
};

/* 2. queue message interface */
class LogMsg {
  public:
    LogMsg() {}

    LogMsg(const std::string& str) : _msg(str.c_str()), _len(str.length()) {}

    LogMsg(const char* str, uint16_t len) : _msg(str), _len(len) {}

    const char* getMsg() { return _msg; }

    [[nodiscard]] uint16_t getMsgLen() const { return _len; }

  private:
    const char* _msg;
    uint16_t _len;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/


/*-------- 5. factories ----------------------------------------------------------------------------------------------*/