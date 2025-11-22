/**
 *******************************************************************************
 * @file    parser.cpp
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

#include "proto-mek.h"

#include <cstring>

#include "FreeRTOS.h"
#include "portable.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/




/* ------- variables -------------------------------------------------------------------------------------------------*/




/* ------- function implement ----------------------------------------------------------------------------------------*/

MekProtocolParser::MekProtocolParser(FrameCallback fc, ErrorCallback ec) : onFrame_(fc), onError_(ec) {}

void MekProtocolParser::feed(const uint8_t* data, uint16_t len) {
    if (!data || len == 0)
        return;
    buffer_.insert(buffer_.end(), data, data + len);
    parse_buffer();
}

void MekProtocolParser::reset() { buffer_.clear(); }

void MekProtocolParser::parse_buffer() {

    constexpr size_t HEADER_SIZE = sizeof(MekProtocolHeader);

    while (buffer_.size() >= HEADER_SIZE) {
        MekProtocolHeader header = {};

        // 1️⃣ 校验 SOF (同步定位)
        uint16_t sof = buffer_[0] | (buffer_[1] << 8); // 小端假设
        if (sof != 0xAA55) {
            // 如果 SOF 不对，滑动 1 字节继续找
            buffer_.erase(buffer_.begin());
            //if (onError_) onError_(MekErrorCode::BAD_SOF);
            continue;
        }

        // 2️⃣ 提取 Header（避免跨界复制）
        memcpy(&header, buffer_.data(), HEADER_SIZE);

        // 3️⃣ 校验长度字段合理性
        if (header.len == 0 || header.len > MAX_PAYLOAD_LEN) {
            // 错误长度：丢弃当前 SOF，继续搜索下一个 SOF
            buffer_.erase(buffer_.begin());
            if (onError_) onError_(MekErrorCode::BAD_LENGTH);
            continue;
        }

        // 总帧长 = 头 + 数据 + CRC(可选)
        size_t frame_total = HEADER_SIZE + header.len + (CRC16_ENABLE ? 2 : 0);

        // 4️⃣ 缓存不足，退出等待更多
        if (buffer_.size() < frame_total)
            return;

        // 5️⃣ CRC 校验（暂存接口）
        if (CRC16_ENABLE) {
            // uint16_t crc_recv = ...
            // uint16_t crc_calc = ...
            // if (crc_recv != crc_calc) {
            //     buffer_.erase(buffer_.begin()); // 不删整包，仅滑动一个字节
            //     if (onError_) onError_(MekErrorCode::CRC_ERROR);
            //     continue;
            // }
        }

        // 6️⃣ 提取 Payload
        std::vector payload(buffer_.begin() + HEADER_SIZE,
                                     buffer_.begin() + HEADER_SIZE + header.len);

        // 7️⃣ 回调 Frame
        if (onFrame_) {
            const auto pf = (MekFrame*)pvPortMalloc(sizeof(MekFrame));
            pf->payload = payload;
            pf->type = header.cmd;
            pf->seq = header.seq;
            pf->self = pf;
            onFrame_(*pf);
        }

        // 8️⃣ 删除整帧数据 ✅安全删除
        buffer_.erase(buffer_.begin(), buffer_.begin() + frame_total);
    }

}
