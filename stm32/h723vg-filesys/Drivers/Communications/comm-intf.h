/**
 *******************************************************************************
 * @file    comm-intf.h
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
 * @author  MekLi🐂
 * @date    2025/8/21
 * @version 1.0
 *******************************************************************************
 */




/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>
#include <string>




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

/**
 * @brief Error Code of communicate interface.
 */
enum class CommErr {
    COMM_ERR_NONE,
    COMM_ERR_TIMEOUT,
    COMM_ERR_BUSY,
    COMM_ERR_INVALID,
    COMM_SUCCESS,
};




/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class IComm {
  public:
    virtual ~IComm()                                         = default;

    virtual CommErr configure()                              = 0;

    virtual CommErr transmit(uint8_t* msg, std::size_t size) = 0;

    virtual CommErr receive(uint8_t* buff, std::size_t size) = 0;

  protected:
    void* _handle;
};


class ICommFcty {
  public:
    virtual ~ICommFcty()                 = default;
    virtual IComm* produce()             = 0;
    virtual IComm* produce(void *handle) = 0;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/





/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

extern ICommFcty* p_cdc_fcty;