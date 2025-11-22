/**
 *******************************************************************************
 * @file    adapter-lfs.cpp
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


/* ------- define --------------------------------------------------------------------------------------------------*/



/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "adapter-lfs.h"
#include "../../Drivers/Devices/w25qxx.h"
#include "../../Middlewares/Third_Party/LittleFs/lfs_util.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/

W25Qxx* LfsAdapter::_w25qxx                 = nullptr;
xSemaphoreHandle LfsAdapter::_waitForFlag   = nullptr;
xSemaphoreHandle LfsAdapter::_waitForTxCplt = nullptr;
xSemaphoreHandle LfsAdapter::_waitForCmd    = nullptr;
xSemaphoreHandle LfsAdapter::_waitForRxCplt = nullptr;




/* ------- function implement ----------------------------------------------------------------------------------------*/



int LfsAdapter::read(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {

    queueCheck();

    static_cast<void>(c);
    uint32_t addr = block * LFS_BLOCK_SIZE + off;
    auto ret = _w25qxx->fastReadQuadOutput(addr, buffer, size);

    if (ret != W25QxxErr::SUCCESS) {
        return LFS_ERR_IO;
    }

    if (xSemaphoreTake(_waitForRxCplt, 1000) != pdTRUE) {
        return LFS_ERR_IO;
    }

    return 0;
}
int LfsAdapter::program(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer,
                        lfs_size_t size) {

    queueCheck();

    if (_waitForFlag == nullptr) {
        return LFS_ERR_IO;
    }

    static_cast<void>(c);

    uint32_t addr = block * LFS_BLOCK_SIZE + off;


    auto p        = static_cast<const uint8_t*>(buffer);
    size_t remain = size;

    while (remain) {
        uint32_t pageOff = addr & (LFS_PROG_SIZE - 1);
        uint32_t chunk   = LFS_PROG_SIZE - pageOff;

        if (chunk > remain) {
            chunk = remain;
        }


        auto ret = _w25qxx->writeEnable();
        if (ret != W25QxxErr::SUCCESS) {
            return LFS_ERR_IO;
        }

        xSemaphoreTake(_waitForTxCplt, 0);
        ret = _w25qxx->pageProgram(addr, buffer, chunk);

        if (ret != W25QxxErr::SUCCESS) {
            return LFS_ERR_IO;
        }

        if (xSemaphoreTake(_waitForTxCplt, 1000) != pdTRUE) {
            return LFS_ERR_IO;
        };

        ret = _w25qxx->asyncWaitForFlag(W25QxxStateEnum::FREE);

        if (ret != W25QxxErr::SUCCESS) {
            return LFS_ERR_IO;
        }


        if (xSemaphoreTake(_waitForFlag, 1000) != pdTRUE) {
            return LFS_ERR_IO;
        };

        addr += chunk;
        p += chunk;
        remain -= chunk;
    }

    return 0;
}

int LfsAdapter::erase(const struct lfs_config* c, lfs_block_t block) {
    (void)c;

    queueCheck();

    uint32_t addr = block * LFS_BLOCK_SIZE;

    if (addr % LFS_BLOCK_SIZE) {
        return LFS_ERR_IO;
    }

    auto ret = _w25qxx->writeEnable();
    if (ret != W25QxxErr::SUCCESS) {
        return LFS_ERR_IO;
    }

    if (xSemaphoreTake(_waitForCmd, 1000) != pdTRUE) {
        return LFS_ERR_IO;
    }
    xSemaphoreTake(_waitForCmd, 0);
    ret = _w25qxx->sectorErase(addr);

    if (ret != W25QxxErr::SUCCESS) {
        return LFS_ERR_IO;
    }

    if (xSemaphoreTake(_waitForCmd, 1000) != pdTRUE) {
        return LFS_ERR_IO;
    }
    _w25qxx->asyncWaitForFlag(W25QxxStateEnum::FREE);


    if (xSemaphoreTake(_waitForFlag, 1000) != pdTRUE) {
        return LFS_ERR_IO;
    }
    return 0;
}

int LfsAdapter::sync(const struct lfs_config* c) {

    queueCheck();

    auto ret = _w25qxx->asyncWaitForFlag(W25QxxStateEnum::FREE);
    if (ret != W25QxxErr::SUCCESS) {
        return LFS_ERR_IO;
    }
    if (xSemaphoreTake(_waitForFlag, 1000) != pdTRUE) {
        return LFS_ERR_IO;
    }
    return 0;
}

inline void LfsAdapter::queueCheck() {
    configASSERT(_waitForFlag);
    configASSERT(_waitForTxCplt);
    configASSERT(_waitForCmd);
}

void LfsAdapter::config(W25Qxx* w25qxx, const SemaphoreHandle_t flg, const SemaphoreHandle_t tx,
                        const SemaphoreHandle_t cmd, const SemaphoreHandle_t rx) {
    _w25qxx        = w25qxx;
    _waitForFlag   = flg;
    _waitForTxCplt = tx;
    _waitForCmd    = cmd;
    _waitForRxCplt = rx;
}
