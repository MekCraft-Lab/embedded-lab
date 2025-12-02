/**
 *******************************************************************************
 * @file    service_explorer.cpp
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


/* ------- define --------------------------------------------------------------------------------------------------*/


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "service_explorer.h"

#include <utility>


/* ------- class prototypes-----------------------------------------------------------------------------------------*/


/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/

const uint8_t endFramePayload[8] = {0xA5, 0x5A, 0x0D, 0x00, 0x07, 0x21, 0xA5, 0x5A};


static uint8_t readBuf[MAX_PAYLOAD_LEN];


/* ------- function implement ----------------------------------------------------------------------------------------*/

ExplorerServer::ExplorerServer(FrameCallback responseHandler, const ErrorHandler& errHandler, lfs_t& lfs)
    : lfs_(lfs), fc_(std::move(responseHandler)) {
    setErrorHandler(errHandler);
    // 文件服务注册
    registerHandler(FrameType::FILE_READ_REQ, [this](MekFrame& frame) { this->fileReadHandler_(frame); }, 4, 256, 2);

    registerHandler(FrameType::DIR_LIST, [this](MekFrame& frame) { this->dirListHandler_(frame); }, 4, 256, 2);

    registerHandler(FrameType::FILE_WRITE_REQ, [this](MekFrame& frame) { this->fileWriteHandler_(frame); }, 4, 256, 2);

    registerHandler(FrameType::FILE_DELETE, [this](MekFrame& frame) { this->fileDeleteHandler_(frame); }, 4, 256, 2);

    registerHandler(FrameType::RENAME, [this](MekFrame& frame) { this->renameHandler_(frame); }, 4, 256, 2);

    registerHandler(FrameType::DIR_CREATE, [this](MekFrame& frame) { this->dirCreateHandler_(frame); }, 4, 256, 2);

    registerHandler(FrameType::CMD, [this](MekFrame& frame) { this->cmdHandler_(frame); }, 4, 256, 2);
}

/**
 * @brief handle the file-read request
 * @param frame Mek通信帧
 */
void ExplorerServer::fileReadHandler_(const MekFrame& frame) {
    // 接收请求
    static FileReadReqFrame req;
    MekFrame f;
    f.type = FrameType::ERROR;
    f.seq  = seq_++;
    f.payload.clear();

    memcpy(&req, frame.payload.data(), sizeof(FileReadReqFrame));

    // 打开文件
    // 考虑到可能同时有多个任务使用文件结构体，每个任务按需创建，防止爆栈，创建静态变量
    static lfs_file_t file{};
    memset(&file, 0,  sizeof(file));


    f.type = FrameType::ERROR;

    int err = lfs_file_open(&lfs_, &file, reinterpret_cast<const char*>(frame.payload.data() + sizeof(req)),
                            LFS_O_RDONLY);
    if (err < 0) {
        f.payload.push_back(static_cast<uint8_t>(err));
        fc_(f);

        vPortFree(frame.self);

        return;
    }

    // 检查偏移
    lfs_soff_t filesize = lfs_file_size(&lfs_, &file);
    if (req.offset >= static_cast<uint8_t>(filesize) && req.offset != 0) {
        f.payload.push_back(static_cast<uint8_t>(MekErrorCode::OFFSET_BEYOND_EDGE));
        fc_(f);
        lfs_file_close(&lfs_, &file);

        vPortFree(frame.self);

        return;
    }

    err = lfs_file_seek(&lfs_, &file, req.offset, LFS_SEEK_SET);

    if (err < 0) {
        f.payload.push_back(static_cast<uint8_t>(err));
        fc_(f);
        lfs_file_close(&lfs_, &file);

        vPortFree(frame.self);

        return;
    }

    // 分包读取
    uint32_t remaining     = req.readLength == 0 ? filesize + 1 : req.readLength;
    uint32_t currentOffset = req.offset;
    uint16_t seqNo         = frame.seq;

    while (remaining > 0) {
        uint16_t chunkLen   = (remaining > MAX_PAYLOAD_LEN) ? MAX_PAYLOAD_LEN : remaining;

        // 读取
        lfs_ssize_t readLen = lfs_file_read(&lfs_, &file, readBuf, chunkLen);
        if (readLen <= 0)
            break; // 读取失败或文件结束

        // 封装响应帧
        f.type = FrameType::FILE_READ_RESP;
        f.payload.insert(f.payload.end(), readBuf, readBuf + readLen);
        f.seq = seq_++;

        // 投递到发送队列
        fc_(f);

        // 更新偏移和剩余长度
        remaining -= readLen;
        currentOffset += readLen;
        seqNo++;
    }

    // 完成帧
    f.type = FrameType::FILE_READ_RESP;
    f.seq  = seq_++;
    f.payload.clear();
    f.payload.insert(f.payload.end(), endFramePayload, endFramePayload + sizeof(endFramePayload));

    fc_(f);

    lfs_file_close(&lfs_, &file);

    vPortFree(frame.self);
    
}


void ExplorerServer::fileDeleteHandler_(const MekFrame& frame) {
    MekFrame f;

    f.type = FrameType::ERROR;
    f.seq  = seq_++;
    f.payload.clear();

    auto err = lfs_remove(&lfs_, reinterpret_cast<const char*>(frame.payload.data()));

    if (err < 0) {
        f.payload.push_back(static_cast<uint8_t>(err));

        fc_(f);
        vPortFree(frame.self);
        return;
    }

    f.type = FrameType::FILE_DELETE_ACK;
    f.payload.insert(f.payload.end(), endFramePayload, endFramePayload + sizeof(endFramePayload));
    fc_(f);

    vPortFree(frame.self);
}


void ExplorerServer::fileWriteHandler_(const MekFrame& frame) {
    static lfs_file_t file;
    memset(&file, 0,  sizeof(file));
    MekFrame f;
    f.type = FrameType::ERROR;
    f.seq  = seq_++;
    f.payload.clear();

    // 解析请求头
    FileWriteReqFrame req;
    memcpy(&req, frame.payload.data(), sizeof(FileWriteReqFrame));

    uint16_t option = req.creat ? LFS_O_CREAT : LFS_O_EXCL;

    // 打开文件
    auto err        = lfs_file_open(&lfs_, &file, reinterpret_cast<const char*>(frame.payload.data() + sizeof(req)),
                                    LFS_O_WRONLY | option);
    if (err != 0) {

        f.payload.push_back(err);
        fc_(f);
        vPortFree(frame.self);
        return;
    }


    // 写入文件
    err = lfs_file_write(&lfs_, &file, frame.payload.data() + sizeof(FileWriteReqFrame) + req.pathLen + 1,
                         frame.payload.size() - sizeof(FileWriteReqFrame) - req.pathLen);
    if (err < 0) {
        f.payload.push_back(static_cast<uint8_t>(err));
        fc_(f);
    }

    err = lfs_file_close(&lfs_, &file);
    if (err != 0) {
        f.type = FrameType::ERROR;
        f.seq  = seq_++;
        f.payload.push_back(static_cast<uint8_t>(err));
        fc_(f);
        vPortFree(frame.self);
        return;
    }


    // 写入反馈帧
    f.type = FrameType::FILE_WRITE_ACK;
    f.seq  = seq_++;
    f.payload.insert(f.payload.end(), endFramePayload, endFramePayload + sizeof(endFramePayload));

    fc_(f);

    vPortFree(frame.self);
}

void ExplorerServer::dirListHandler_(const MekFrame& frame) {

    // 打开文件夹
    // 防止爆栈，静态变量
    static lfs_dir_t dir;
    static lfs_info info;

    memset(&dir, 0, sizeof(dir));
    memset(&info, 0, sizeof(info));

    int err = lfs_dir_open(&lfs_, &dir, (const char*)frame.payload.data());


    if (err < 0) {
        std::vector<uint8_t> e;
        e.insert(e.end(), static_cast<uint8_t>(err));
        MekFrame f(FrameType::ERROR, seq_++, e);
        fc_(f);
        vPortFree(frame.self);
        return;
    }

    MekFrame f;

    f.type = FrameType::DIR_LIST_RESP;
    f.seq  = seq_++;

    while (lfs_dir_read(&lfs_, &dir, &info) > 0) {
        // 跳过 "." 和 ".."
        if (info.name[0] == '.' && (info.name[1] == '\0' || (info.name[1] == '.' && info.name[2] == '\0'))) {
            continue;
        }

        uint16_t nameLen = strlen(info.name) + 1;
        f.payload.push_back(info.type);
        f.payload.push_back(info.size & 0x000000FF);
        f.payload.push_back((info.size & 0x0000FF00) >> 8);
        f.payload.push_back((info.size & 0x00FF0000) >> 16);
        f.payload.push_back((info.size & 0xFF000000) >> 24);

        f.payload.insert(f.payload.end(), info.name, info.name + nameLen);

        fc_(f);

        f.payload.clear();
    }

    lfs_dir_close(&lfs_, &dir);


    f.payload.insert(f.payload.end(), endFramePayload, endFramePayload + 8);
    fc_(f);
    vPortFree(frame.self);
}


void ExplorerServer::renameHandler_(const MekFrame& frame) {
    MekFrame f;

    f.type = FrameType::ERROR;
    f.seq  = seq_++;

    RenReqFrame req;

    memcpy(&req, frame.payload.data(), sizeof(RenReqFrame));

    if (req.newPathLen + req.originalPathLen != frame.payload.size()) {
        f.payload.push_back(static_cast<uint8_t>(MekErrorCode::PATH_LENGTH_BEYOND));
        fc_(f);
        vPortFree(frame.self);
        return;
    }

    auto pOldPath = reinterpret_cast<const char*>(frame.payload.data() + sizeof(req) + 1);
    auto pNewPath = reinterpret_cast<const char*>(frame.payload.data() + sizeof(req) + req.originalPathLen + 2);

    auto err      = lfs_rename(&lfs_, pOldPath, pNewPath);

    if (err < 0) {
        f.payload.push_back(static_cast<uint8_t>(err));
        fc_(f);
        vPortFree(frame.self);
        return;
    }

    f.type = FrameType::RENAME_ACK;
    f.payload.insert(f.payload.end(), endFramePayload, endFramePayload + sizeof(endFramePayload));
    fc_(f);
    vPortFree(frame.self);
}

void ExplorerServer::dirCreateHandler_(const MekFrame& frame) {
    MekFrame f;

    f.type   = FrameType::ERROR;
    f.seq    = seq_++;

    auto err = lfs_mkdir(&lfs_, reinterpret_cast<const char*>(frame.payload.data()));
    if (err < 0) {
        f.payload.push_back(static_cast<uint8_t>(err));
        fc_(f);
        vPortFree(frame.self);
        return;
    }

    f.type = FrameType::DIR_CREATE_ACK;
    f.payload.insert(f.payload.end(), endFramePayload, endFramePayload + sizeof(endFramePayload));
    fc_(f);
    vPortFree(frame.self);
}

void ExplorerServer::cmdHandler_(const MekFrame &frame) {

    MekFrame f;
    CtrlReqFrame req;


    f.type   = FrameType::ERROR;
    f.seq    = seq_++;

    f.payload.clear();

    memcpy(&req, frame.payload.data(), sizeof(CtrlReqFrame));

    if (req.payloadLen != frame.payload.size() - sizeof(req)) {
        f.payload.push_back(static_cast<uint8_t>(MekErrorCode::BAD_LENGTH));
        fc_(f);
        vPortFree(frame.self);
        return;
    }

    switch (req.ctrlCode) {

        case MekCtrlCode::HAND_SHAKE_REQ : {

            if (req.payloadLen != sizeof(HandShakeFrame)) {
                f.payload.push_back(static_cast<uint8_t>(MekErrorCode::BAD_LENGTH));
                fc_(f);
                vPortFree(frame.self);
                return;
            }

            HandShakeFrame handframe;

            memcpy(&handframe, frame.payload.data() + sizeof(CtrlReqFrame), sizeof(HandShakeFrame));
            if (handframe.version != VERSION) {
                f.payload.push_back(static_cast<uint8_t>(MekErrorCode::BAD_VERSION));
                fc_(f);
                vPortFree(frame.self);
                return;
            }

            f.type = FrameType::CMD;
            f.seq    = seq_++;

            // 控制帧帧头
            req.ctrlCode = MekCtrlCode::HAND_SHAKE_RESP;
            f.payload.insert(f.payload.end(), (uint8_t*)&req, (uint8_t*)&req + sizeof(CtrlReqFrame));
            f.payload.insert(f.payload.end(), (uint8_t*)&handframe, (uint8_t*)&handframe + sizeof(HandShakeFrame));


            fc_(f);

            vPortFree(frame.self);
            return;
        }
        default: ;
    }

}
