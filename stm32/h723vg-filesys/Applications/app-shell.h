/**
 *******************************************************************************
 * @file    app-shell.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/


/* I. interface */
#include "app-intf.h"

/* II. OS */
#include "message_buffer.h"

/* III. middlewares */
#include "../../Middlewares/Third_Party/LetterShell/log/log.h"
#include "../../Middlewares/Third_Party/LetterShell/shell_cpp.h"
#include "Utils/ringBuffer.h"

/* IV. drivers */
#include "../../Drivers/Communications/comm-intf.h"

/* V. standard lib */
#include <cstring>




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class ShellApp final : public StaticAppBase {
  public:
    ShellApp();

    void init() override;

    void run() override;

    void sendMsg(uint8_t *data, uint16_t len);

    /************ setter & getter ***********/
    static ShellApp& instance();
    Shell* getShell() { return &_shell; }
    Log& getLog() { return _log; }

  private:
    /* message interface */
    RingBuffer _rb1;
    RingBuffer _rb2;
    uint8_t _index;

    Shell _shell;
    Log _log;


    friend int16_t shellWrite(char* data, uint16_t len);
    friend void logWrite(char* data, int16_t len);
};

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/





/*-------- 5. factories ----------------------------------------------------------------------------------------------*/
