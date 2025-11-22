/**
 *******************************************************************************
 * @file    app-file.h
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

/* III. middlewares */
#include "app-console.h"
#include "app-shell.h"
#include "../Adapters/adapter-lfs.h"

/* IV. drivers */
#include "../../Drivers/Devices/w25qxx.h"
#include "octospi.h"



/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

enum class FsOptEnum {
    NONE,
    DIR_OPEN,
    DIR_READ,
    DIR_MAKE,
    FILE_OPEN,
    FILE_READ,
    FILE_WRITE,
    FILE_WRITE_AND_MAKE,
    FILE_CREATE_NEW,
    FILE_APPEND,
    REMOVE,
};

/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class FsMsg {
  public:
    FsMsg(FsOptEnum opt, const char* path, uint8_t* buffer, uint16_t bufferLen) {
        _opt       = opt;
        _path      = path;
        _buffer    = buffer;
        _bufferLen = bufferLen;

        for (uint8_t i = 0; i < 0xFF; i++) {
            if (path[i] == 0x00) {
                _pathLen = i;
            }
        }

        _cb = xSemaphoreCreateBinary();
    }

    FsMsg(FsOptEnum opt, const char* path) {
        _opt  = opt;
        _path = path;

        for (uint8_t i = 0; i < 0xFF; i++) {
            if (path[i] == 0x00) {
                _pathLen = i;
            }
        }

        _cb = xSemaphoreCreateBinary();
    }

    ~FsMsg() {
        if (_cb) {
            vSemaphoreDelete(_cb);
        }
    }

    FsOptEnum getOpt() const { return _opt; }
    const char* getPath() const { return _path; }
    uint8_t getPathLen() const { return _pathLen; }
    uint8_t* getBuffer() const { return _buffer; }
    uint16_t getBufferLen() const { return _bufferLen; }
    bool waitForReply() const { return xSemaphoreTake(_cb, 5000) == pdTRUE; }
    void finishReply() const { xSemaphoreGive(_cb); }

    void setReturn(const uint16_t ret) { _ret = ret; }
    int16_t getReturn() const { return _ret; }

  private:
    FsOptEnum _opt       = FsOptEnum::NONE;
    const char* _path    = nullptr;
    uint8_t _pathLen     = 0;
    uint8_t* _buffer     = nullptr;
    uint16_t _bufferLen  = 0;
    int16_t _ret          = 0;
    xSemaphoreHandle _cb = nullptr;
};




class FileApp : public StaticAppBase {

  public:
    FileApp();
    static FileApp& instance();

    void init() override;

    void run() override;

    int16_t readFile(const char* path, uint8_t* buffer, uint16_t bufferLen) const;
    int8_t writeAndMakeFile(const char *path, const char *buffer, uint16_t bufferLen) const;
    int8_t writeFile(const char* path, const char* buffer, uint16_t bufferLen) const;
    int8_t appendFile(const char* path, const char* buffer, uint16_t bufferLen) const;
    int8_t createFile(const char* path) const;

    int16_t readDir(const char *path, uint8_t *buffer, uint16_t bufferLen) const;
    int8_t remove(const char* path);
    int8_t makeDir(const char* path);

    int8_t openDir(const char *path);

    int8_t rename(const char* path);
    int8_t state(const char* path);
    int8_t sysVolume();




  private:
    W25Qxx _flash;
    xQueueHandle _request;
    xSemaphoreHandle _waitForReceiveLock;
    xSemaphoreHandle _waitForTransmitLock;
    xSemaphoreHandle _waitForCmd;
    xSemaphoreHandle _waitForFlag;
    LfsAdapter _lfsAdapter;
    float _runTime;

    friend void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_StatusMatchCallback(OSPI_HandleTypeDef* hospi);
    friend void waitForRxCplt();
};

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/


/*-------- 5. factories ----------------------------------------------------------------------------------------------*/
