/**
 *******************************************************************************
 * @file    app-fileSystem.cpp
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
 * @date    2025/8/22
 * @version 1.0
 *******************************************************************************
 */




/* ------- define --------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "app-fileSystem.h"

#include "app-host.h"
#include "octospi.h"
#include "usart.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/

// int16_t fileHandle(FsOptEnum opt, const char* path, uint8_t* buff, uint16_t buffLen);
// int16_t dirHandle(FsOptEnum opt, const char* path, uint8_t* buff, uint16_t buffLen);
// int16_t removeHandle(const char* path);



/* ------- import interface ------------------------------------------------------------------------------------------*/

extern "C" {}


/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "FileSys"

#define APPLICATION_STACK_SIZE 1024

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];




/* ------- message interface attribute -------------------------------------------------------------------------------*/

#define STREAM_BUFFER_SIZE 1024

static uint8_t sbStg[STREAM_BUFFER_SIZE];





/* ------- variables -------------------------------------------------------------------------------------------------*/

static FileSysApp fileSysApp;
static HostApp& host = HostApp::instance();

__attribute__((section(".dma_pool"))) uint8_t rxBuf[512];

uint8_t frameBuffer[270];
uint8_t respBuffer[270];
static uint8_t lfs_read_buf[LFS_CACHE_SIZE];
static uint8_t lfs_prog_buf[LFS_CACHE_SIZE];
static uint8_t lfs_lookahead_buf[LFS_LOOKAHEAD_SIZE];
static lfs_t littleFsInfo;
static lfs_config littleFsConfig{};
/* ------- function implement ----------------------------------------------------------------------------------------*/

FileSysApp::FileSysApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr),
      _service([this](MekFrame& f) { host.frameResp(f); }, [this](ServerError e) { serverErr(e); }, littleFsInfo),
      _parser([this](MekFrame& f) { _service.dispatch(f.type, f); },
              [this](MekErrorCode err) {
                  std::vector<uint8_t> e;
                  e.insert(e.end(), static_cast<uint8_t>(err));
                  MekFrame f(FrameType::ERROR, 0, e);
                  host.frameResp(f);
              }),
      _flash(&hospi1) {

    _waitForReceiveLock  = xSemaphoreCreateBinary();
    _waitForTransmitLock = xSemaphoreCreateBinary();
    _waitForFlag         = xSemaphoreCreateBinary();
    _waitForCmd          = xSemaphoreCreateBinary();
    _sbHandle            = xStreamBufferCreateStatic(STREAM_BUFFER_SIZE, 1, sbStg, &_stm);
    configASSERT(_waitForReceiveLock);
    configASSERT(_waitForTransmitLock);
    configASSERT(_waitForFlag);
    configASSERT(_waitForCmd);

    LfsAdapter::config(&_flash, _waitForFlag, _waitForTransmitLock, _waitForCmd, _waitForReceiveLock);
}

FileSysApp& FileSysApp::instance() { return fileSysApp; }


void FileSysApp::init() {


    // 等待控制台任务完成初始化
    host.waitInit();

    // 初始化串口
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxBuf, 512);

    // _flash.writeEnable();
    // vTaskDelay(1);
    //
    // _flash.chipErase();
    // xSemaphoreTake(_waitForCmd, portMAX_DELAY);
    //
    // _flash.asyncWaitForFlag(W25QxxStateEnum::FREE);
    // xSemaphoreTake(_waitForFlag, portMAX_DELAY);




    _flash.enquireSfdpRegisterAsync();
    xSemaphoreTake(_waitForReceiveLock, portMAX_DELAY);
    _flash.asyncRxCallback();
    uint16_t sfdpHH = _flash.getSFDP() >> 48;
    uint16_t sfdpHL = _flash.getSFDP() >> 32;
    uint16_t sfdpLH = _flash.getSFDP() >> 16;
    uint16_t sfdpLL = _flash.getSFDP();
    host.println("[W25Q64] Initialization infomation:\r\nSFDP Register\t\t: %04X %04X %04X %04X", sfdpHH, sfdpHL,
                 sfdpLH, sfdpLL);


    auto r = _flash.writeEnable();
    r      = _flash.writeRegister(W25QxxRegisterEnum::STATUS_REGISTER_1, 0x00);
    (void)r;
    vTaskDelay(10);

    r = _flash.writeEnable();
    r = _flash.writeRegister(W25QxxRegisterEnum::STATUS_REGISTER_2, 0x02);
    (void)r;
    vTaskDelay(10);

    r = _flash.writeEnable();
    r = _flash.writeRegister(W25QxxRegisterEnum::STATUS_REGISTER_3, 0x00);
    (void)r;
    vTaskDelay(10);

    _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_1);
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();

    _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_2);
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();

    _flash.enquireStatusRegisterAsync(W25QxxRegisterEnum::STATUS_REGISTER_3);
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();

    host.println("Status Register 1\t: 0x%02X\r\nStatus Register 2\t: 0x%02X\r\nStatus Register 3\t: 0x%02X",
                 _flash.getSR1(), _flash.getSR2(), _flash.getSR3());

    _flash.enquireJedecIdAsync();
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();
    host.println("Manufacturer ID\t\t: %02X\r\nDevice ID - 16\t\t: %04X", _flash.getMID(), _flash.getDevID16());

    _flash.enquireDeviceIdAsync();
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();
    host.println("Device ID - 8\t\t: %02X", _flash.getDevID8());

    _flash.enquireUniqueIdAsync();
    xSemaphoreTake(_waitForReceiveLock, 50);
    _flash.asyncRxCallback();
    uint64_t uniqueID   = _flash.getUniqueID();
    uint16_t uniqueIDHH = uniqueID >> 48;
    uint16_t uniqueIDHL = uniqueID >> 32;
    uint16_t uniqueIDLH = uniqueID >> 16;
    uint16_t uniqueIDLL = uniqueID;
    host.println("Unique ID\t\t: %04X %04X %04X %04X\r\n", uniqueIDHH, uniqueIDHL, uniqueIDLH, uniqueIDLL);




    littleFsConfig.read             = LfsAdapter::read;
    littleFsConfig.prog             = LfsAdapter::program;
    littleFsConfig.erase            = LfsAdapter::erase;
    littleFsConfig.sync             = LfsAdapter::sync;
    littleFsConfig.read_size        = LFS_READ_SIZE;
    littleFsConfig.prog_size        = LFS_PROG_SIZE;
    littleFsConfig.block_size       = LFS_BLOCK_SIZE;
    littleFsConfig.block_count      = LFS_BLOCK_COUNT;
    littleFsConfig.block_cycles     = LFS_BLOCK_CYCLES;
    littleFsConfig.cache_size       = LFS_CACHE_SIZE;
    littleFsConfig.lookahead_size   = LFS_LOOKAHEAD_SIZE;
    littleFsConfig.read_buffer      = lfs_read_buf;
    littleFsConfig.prog_buffer      = lfs_prog_buf;
    littleFsConfig.lookahead_buffer = lfs_lookahead_buf;
    littleFsConfig.compact_thresh   = -1;

    auto err                        = lfs_mount(&littleFsInfo, &littleFsConfig);


    if (err) {

        host.println("LittleFS mount failed");

        std::vector<uint8_t> e;
        e.insert(e.end(), static_cast<uint8_t>(MekErrorCode::NO_FILE_SYSTEM));
        MekFrame f(FrameType::ERROR, 0, e);

        host.frameResp(f);

        host.println("formatting flash...");

        lfs_format(&littleFsInfo, &littleFsConfig);

        err = lfs_mount(&littleFsInfo, &littleFsConfig);

        if (err) {
            host.println("format failed");
            f.seq++;
            f.payload.at(0) = static_cast<uint8_t>(MekErrorCode::MOUNT_FILE_SYSTEM_FAILED);
            host.frameResp(f);

            vTaskDelete(nullptr);
        }

        host.println("format success");
    }
    host.println("mount LittleFs success");
}



void FileSysApp::run() {
    uint16_t frameLen  = xStreamBufferReceive(_sbHandle, frameBuffer, 270, portMAX_DELAY);
    uint32_t startTime = htim2.Instance->CNT;
    _parser.feed(frameBuffer, frameLen);
    uint32_t execTime = htim2.Instance->CNT - startTime;
    host.println("协议解析耗时:%dμs", execTime);
}

void FileSysApp::serverErr(ServerError sr) {
    static uint16_t seq = 0;
    std::vector<uint8_t> e;
    if (sr == ServerError::SERVICE_BUSY) {
        e.insert(e.end(), static_cast<uint8_t>(MekErrorCode::BUSY));
    } else if (sr == ServerError::UNKNOWN_FRAME_TYPE) {
        e.insert(e.end(), static_cast<uint8_t>(MekErrorCode::CMD_NOT_FIND));
    } else {
        return;
    }

    MekFrame f(FrameType::ERROR, seq++, e);
    host.frameResp(f);
}





uint8_t FileSysApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) {
    xStreamBufferSend(_sbHandle, msg, size, timeout);
    return 0;
}

uint8_t FileSysApp::rxMsg(void* msg, uint16_t size) {
    xStreamBufferSendFromISR(_sbHandle, msg, size, nullptr);
    return 0;
}



/*---- interrupt -----------------------------------------------------------------------------------------------------*/

void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef* hospi) {

    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileSysApp._waitForReceiveLock, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef* hospi) {

    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileSysApp._waitForCmd, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef* hospi) {

    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileSysApp._waitForTransmitLock, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_OSPI_StatusMatchCallback(OSPI_HandleTypeDef* hospi) {
    if (hospi == &hospi1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(fileSysApp._waitForFlag, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t Size) {
    if (huart == &huart1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xStreamBufferSendFromISR(FileSysApp::instance()._sbHandle, rxBuf, Size, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxBuf, 512);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    if (huart == &huart1) {
        if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        }
        if (huart->ErrorCode & HAL_UART_ERROR_FE)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        if (huart->ErrorCode & HAL_UART_ERROR_NE)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        if (huart->ErrorCode & HAL_UART_ERROR_PE)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);

        // 1. 停止接收
        HAL_UART_AbortReceive(huart);

        // 2. 关闭UART
        __HAL_UART_DISABLE(huart);

        // 3. 清除所有错误标志
        __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_OREFLAG(huart);

        // 4. 重新启用UART
        __HAL_UART_ENABLE(huart);

        // 5. 重新启动接收
        HAL_UARTEx_ReceiveToIdle_DMA(huart, rxBuf, 512);
    }
}
