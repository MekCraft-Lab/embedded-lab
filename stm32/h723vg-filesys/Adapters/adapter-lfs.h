/**
 *******************************************************************************
 * @file    adapter-lfs.h
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
 * @date    2025/8/25
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/
#pragma once




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../../Middlewares/Third_Party/LittleFs/lfs.h"
#include "../Drivers/Devices/w25qxx.h"
#include "FreeRTOS.h"
#include "semphr.h"




/*-------- 2. enum & define ------------------------------------------------------------------------------------------*/

#define LFS_BLOCK_SIZE     4096
#define LFS_BLOCK_COUNT    2048
#define LFS_PROG_SIZE      256
#define LFS_READ_SIZE      64
#define LFS_CACHE_SIZE     256
#define LFS_LOOKAHEAD_SIZE 256
#define LFS_BLOCK_CYCLES   500





/*-------- 3. interface ---------------------------------------------------------------------------------------------*/
class LfsAdapter {
  public:

    static int read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

    static int program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer,
                       lfs_size_t size);

    static int erase(const struct lfs_config* c, lfs_block_t block);

    static int sync(const struct lfs_config* c);

    static inline void queueCheck();

    static void config(W25Qxx *w25qxx, SemaphoreHandle_t flg, SemaphoreHandle_t tx, SemaphoreHandle_t cmd, SemaphoreHandle_t rx);

  private:
    static W25Qxx* _w25qxx;
    static xSemaphoreHandle _waitForFlag;
    static xSemaphoreHandle _waitForTxCplt;
    static xSemaphoreHandle _waitForCmd;
    static xSemaphoreHandle _waitForRxCplt;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/


/*-------- 5. factories ----------------------------------------------------------------------------------------------*/
