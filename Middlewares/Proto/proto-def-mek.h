/**
 *******************************************************************************
 * @file    parser-def-mek.h
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



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>
#include <functional>
#include <vector>


/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/

#define FRAME_HEADER    0xAA55
#define MAX_PAYLOAD_LEN 512
#define CRC16_ENABLE    false

#define VERSION 0x00010000



/**
 * @brief FrameType represents the type of protocol frame used in UART communication.
 */


enum class MekCtrlCode : uint8_t;

enum class FrameType : uint8_t {

    NONE            = 0x00,

    // ---------- 文件操作 ----------
    FILE_READ_REQ   = 0x01, ///< Description: Request to read a file
                            ///< Payload: File path, offset, length
                            ///< Note: Worker reads from LittleFS and responds with FILE_READ_RESP

    FILE_READ_RESP  = 0x02, ///< Description: Response containing file data
                            ///< Payload: File data + optional sequence number for multi-part
                            ///< Note: Feed reassembles multipart frames automatically

    FILE_WRITE_REQ  = 0x03, ///< Description: Request to write a file
                            ///< Payload: File path, offset, length + file data
                            ///< Note: Worker writes to LittleFS and responds with FILE_WRITE_ACK

    FILE_WRITE_ACK  = 0x04, ///< Description: Acknowledgment of file write
                            ///< Payload: Status code (0=success, non-zero=error)
                            ///< Note: Indicates result of FILE_WRITE_REQ

    FILE_DELETE     = 0x05, ///< Description: Request to delete a file
                            ///< Payload: File path
                            ///< Note: Worker responds with FILE_DELETE_ACK

    FILE_DELETE_ACK = 0x06, ///< Description: Response to FILE_DELETE
                            ///< Payload: Status code (0=success, non-zero=error)

    RENAME     = 0x0E, ///< Description: Request to rename/move a file
                            ///< Payload: Old file path + New file path
                            ///< Note: Worker calls lfs_rename() and responds with FILE_RENAME_ACK

    RENAME_ACK = 0x0F, ///< Description: Response to FILE_RENAME
                            ///< Payload: Status code (0=success, non-zero=error)

    // ---------- 目录操作 ----------
    DIR_CREATE      = 0x12, ///< Description: Request to create a directory
                            ///< Payload: Directory path
                            ///< Note: Worker calls lfs_mkdir() and responds with DIR_CREATE_ACK

    DIR_CREATE_ACK  = 0x13, ///< Description: Response to DIR_CREATE
                            ///< Payload: Status code (0=success, non-zero=error)

    DIR_DELETE      = 0x14, ///< Description: Request to delete a directory
                            ///< Payload: Directory path
                            ///< Note: Worker calls lfs_remove() and responds with DIR_DELETE_ACK

    DIR_DELETE_ACK  = 0x15, ///< Description: Response to DIR_DELETE
                            ///< Payload: Status code (0=success, non-zero=error)


    DIR_LIST        = 0x18, ///< Description: Request to list directory contents
                            ///< Payload: Directory path
                            ///< Note: Worker responds with DIR_LIST_RESP

    DIR_LIST_RESP   = 0x19, ///< Description: Response containing directory entries
                            ///< Payload: File and sub-directory names

    // ---------- 系统与控制 ----------
    HEARTBEAT       = 0x20, ///< Description: Heartbeat frame for link keep-alive
                            ///< Payload: Optional timestamp or random number
                            ///< Note: Ensures MCU and host remain connected

    ERROR           = 0x21, ///< Description: Error reporting frame
                            ///< Payload: Error code + description
                            ///< Note: Used to report CRC, protocol, or file operation errors

    CMD             = 0x22, ///< Description: System command frame
                           ///< Payload: Command ID + parameters
                           ///< Note: Can be extended for operations like reboot, status query
};



/*-------- 3. interface ---------------------------------------------------------------------------------------------*/

class MekFrame {
  public:
    MekFrame(FrameType typeParam, uint16_t seqParam, std::vector<uint8_t> payloadParam)
        : type(typeParam), seq(seqParam), payload(payloadParam) {};
    MekFrame(){}
    FrameType type = FrameType::NONE;
    uint16_t seq   = 0;
    std::vector<uint8_t> payload;
    MekFrame* self;
};


// 文件读取请求帧
#pragma pack(push, 1)
typedef struct {
    uint32_t offset;
    uint16_t readLength;
} FileReadReqFrame;
#pragma pack(pop)

// 文件写入请求帧
#pragma pack(push, 1)
typedef struct {
    uint8_t pathLen;
    uint8_t creat;
} FileWriteReqFrame;
#pragma pack(pop)


// 重命名请求帧
#pragma pack(push, 1)
typedef struct {
    uint8_t originalPathLen;
    uint8_t newPathLen;
} RenReqFrame;
#pragma pack(pop)

// 控制命令请求帧
#pragma pack(push, 1)
typedef struct {
    MekCtrlCode ctrlCode;
    uint16_t payloadLen;
} CtrlReqFrame;
#pragma pack(pop)

// 控制命令-握手帧
#pragma pack(push, 1)
typedef struct {
    uint32_t randomNumber;
    uint32_t version;
} HandShakeFrame;
#pragma pack(pop)


// 总帧帧头
#pragma pack(push, 1)
 typedef struct {
    uint16_t sof;
    uint16_t seq;
    FrameType cmd;
    uint16_t len;
} MekProtocolHeader;
#pragma pack(pop)


enum class MekCtrlCode : uint8_t{
    NONE = 0x00,
    HAND_SHAKE_REQ = 0x01,
    HAND_SHAKE_RESP = 0x02,
};



enum class MekErrorCode {
    NONE = 0x00,
    NO_FILE_SYSTEM = 0x01,
    MOUNT_FILE_SYSTEM_FAILED = 0x02,
    PAYLOAD_TOO_BIG = 0x03,
    FILE_OPEN_FAILED = 0x04,
    OFFSET_BEYOND_EDGE = 0x05,
    BUSY = 0x06,
    CMD_NOT_FIND = 0x07,
    BAD_LENGTH = 0x08,
    BAD_SOF = 0x09,
    DIR_OPEN_FAILED = 0x0A,
    MEMORY_ALLOCATION_FAILED = 0x0B,
    FILE_WRITE_FILED = 0x0C,
    FILE_CLOSE_FILED = 0x0D,
    PATH_LENGTH_BEYOND = 0x0E,
    OFFSET_FAILED = 0x0F,
    FILE_REMOVE_FAILED = 0x10,
    RENAME_FAILED = 0x11,
    DIR_CREATE_FAILED = 0x12,
    BAD_VERSION = 0x13,
};

using FrameCallback = std::function<void(MekFrame&)>;

using ErrorCallback = std::function<void(MekErrorCode)>;



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/


/*-------- 5. factories ----------------------------------------------------------------------------------------------*/