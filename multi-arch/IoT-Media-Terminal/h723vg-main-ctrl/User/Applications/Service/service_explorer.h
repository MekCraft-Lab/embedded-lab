/**
 *******************************************************************************
 * @file    service_explorer.h
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
 * @date    2025/10/17
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once



/*-------- includes --------------------------------------------------------------------------------------------------*/

/* 1. interface */
#include "server-intf.h"

/* 2. application */
#include "Middlewares/Proto/proto-def-mek.h"
#include "Middlewares/LittleFs/Adapters/adapter-lfs.h"



/*-------- typedef ---------------------------------------------------------------------------------------------------*/

class ExplorerServer : public IServer<FrameType, MekFrame> {
  public:
    ExplorerServer(FrameCallback responseHandler, const ErrorHandler&  errHandler, lfs_t& lfs);

  private:
    // ---------- 回调函数 ----------
    void fileReadHandler_(const MekFrame& f);
    void fileWriteHandler_(const MekFrame& f);
    void fileDeleteHandler_(const MekFrame& f);
    void dirCreateHandler_(const MekFrame& f);
    void renameHandler_(const MekFrame& f);
    void dirListHandler_(const MekFrame& f);
    void heartbeatHandler_(const MekFrame& f);
    void cmdHandler_(const MekFrame& f);
    void errorHandler_(const MekFrame& f);


    lfs_t& lfs_;
    uint16_t seq_{};

    FrameCallback fc_;
};


/*-------- define ----------------------------------------------------------------------------------------------------*/


/*-------- macro -----------------------------------------------------------------------------------------------------*/


/*-------- variables -------------------------------------------------------------------------------------------------*/


/*-------- function prototypes ---------------------------------------------------------------------------------------*/