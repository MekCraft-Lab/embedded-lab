/**
 *******************************************************************************
 * @file    app-fileSystem.h
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
#include "semphr.h"
#include "stream_buffer.h"

/* III. middlewares */
#include "../Middlewares/Proto/proto-mek.h"
#include "../Middlewares/Third_Party/LittleFs/lfs.h"
#include "../Middlewares/Third_Party/LittleFs/Adapters/adapter-lfs.h"

/* IV. drivers */
#include "../Drivers/Devices/w25qxx.h"
#include "tim.h"

#include "Service/service_explorer.h"


/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

// enum class FsOptEnum {
//     NONE,
//     DIR_OPEN,
//     DIR_READ,
//     DIR_MAKE,
//     FILE_OPEN,
//     FILE_READ,
//     FILE_WRITE,
//     FILE_WRITE_AND_MAKE,
//     FILE_CREATE_NEW,
//     FILE_APPEND,
//     REMOVE,
// };

/*-------- 3. interface ---------------------------------------------------------------------------------------------*/
//
// class FsMsg {
//   public:
//     FsMsg(FsOptEnum opt, const char* path, uint8_t* buffer, uint16_t bufferLen) {
//         _opt       = opt;
//         _path      = path;
//         _buffer    = buffer;
//         _bufferLen = bufferLen;
//
//         for (uint8_t i = 0; i < 0xFF; i++) {
//             if (path[i] == 0x00) {
//                 _pathLen = i;
//             }
//         }
//
//         _cb = xSemaphoreCreateBinary();
//     }
//
//     FsMsg(FsOptEnum opt, const char* path) {
//         _opt  = opt;
//         _path = path;
//
//         for (uint8_t i = 0; i < 0xFF; i++) {
//             if (path[i] == 0x00) {
//                 _pathLen = i;
//             }
//         }
//
//         _cb = xSemaphoreCreateBinary();
//     }
//
//     ~FsMsg() {
//         if (_cb) {
//             vSemaphoreDelete(_cb);
//         }
//     }
  //
  //   FsOptEnum getOpt() const { return _opt; }
  //   const char* getPath() const { return _path; }
  //   uint8_t getPathLen() const { return _pathLen; }
  //   uint8_t* getBuffer() const { return _buffer; }
  //   uint16_t getBufferLen() const { return _bufferLen; }
  //   bool waitForReply() const { return xSemaphoreTake(_cb, 5000) == pdTRUE; }
  //   void finishReply() const { xSemaphoreGive(_cb); }
  //
  //   void setReturn(const uint16_t ret) { _ret = ret; }
  //   int16_t getReturn() const { return _ret; }
  //
  // private:
  //   FsOptEnum _opt       = FsOptEnum::NONE;
  //   const char* _path    = nullptr;
  //   uint8_t _pathLen     = 0;
  //   uint8_t* _buffer     = nullptr;
  //   uint16_t _bufferLen  = 0;
  //   int16_t _ret          = 0;
  //
  //   xSemaphoreHandle _cb = nullptr;
// };


class FileDesc {
public:
    uint8_t sof = 0xA5;
    uint16_t length;
    char name[10];

    FileDesc(uint16_t len, const char* fileName) {
        length = len;
        strcpy(name, fileName);
    }
    FileDesc() {}
};

class FileSysApp : public StaticAppBase {

  public:
    FileSysApp();
    static FileSysApp& instance();

    void init() override;

    void run() override;

    uint8_t deconstruction(uint8_t *buf, size_t len);

    uint8_t getFile(uint8_t ISR, uint16_t fileLen, const char *fileName, uint8_t *pFile);

    uint8_t getFile(uint16_t fileLen, const char *fileName, uint8_t *pFile);

    uint8_t rxMsg(void* msg, uint16_t size, TickType_t timeout) override;
    uint8_t rxMsg(void* msg, uint16_t size) override;


    [[nodiscard]] xSemaphoreHandle getReceiveLock() { return _waitForReceiveLock;}
    [[nodiscard]] xSemaphoreHandle getTransmitLock()  { return _waitForTransmitLock;}
    [[nodiscard]] xSemaphoreHandle getCmdLock()  {return _waitForCmd;}
    [[nodiscard]] xSemaphoreHandle getFlagLock()  {return _waitForFlag;}
    [[nodiscard]] W25Qxx& getFlash() {return _flash;}

  private:
    ExplorerServer _service; // 协议处理服务器

    MekProtocolParser _parser; // 协议解析器




    W25Qxx _flash;
    xQueueHandle _request;
    xSemaphoreHandle _waitForReceiveLock;
    xSemaphoreHandle _receiveFinished;
    xSemaphoreHandle _waitForTransmitLock;
    xSemaphoreHandle _waitForCmd;
    xSemaphoreHandle _waitForFlag;
    StreamBufferHandle_t _sbHandle;
    StaticStreamBuffer_t _stm;
    float _runTime;

    static void serverErr(ServerError sr);

    friend void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef*);
    friend void HAL_OSPI_StatusMatchCallback(OSPI_HandleTypeDef* hospi);
    friend void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size);
    friend void waitForRxCplt();
};