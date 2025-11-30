/**
 *******************************************************************************
 * @file    drv-ili9841.cpp
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
 * @date    2025/11/15
 * @version 1.0
 *******************************************************************************
 */


/* ------- define --------------------------------------------------------------------------------------------------*/




/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "drv-ili9481.h"




/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

const uint8_t powerSettingParam[]    = {0x07, 0x42, 0x18};
const uint8_t vcomSettingParam[]     = {0x00, 0x18, 0x04};
const uint8_t normalModePowerParam[] = {0x01, 0x02};
const uint8_t panelDrivingParam[]    = {SCAN_MODE << 2 | GRAYSCALE_INVERSION << 4, DISPLAY_LINES / 8 - 1, 00, 0x02,
                                        0x00};
const uint8_t frameRateParame[]      = {SCAN_FPS};
const uint8_t gammaParam[]           = {0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00};
const uint8_t columnAddr[]           = __START_AND_END_TO_BYTE_ARRAY(0, 320);
const uint8_t pageAddr[]             = __START_AND_END_TO_BYTE_ARRAY(0, 480);

extern uint32_t exec_time_us;
/* ------- function implement ----------------------------------------------------------------------------------------*/

ILI9481::ILI9481(delayFunc delayMs) {
    if (delayMs != nullptr) {
        _delayMs = delayMs;
    }

}

static inline void FMC_Write16(uint32_t addr, uint16_t data) {
    __asm volatile(
        "strh %1, [%0]"
        :
        : "r" (addr), "r" (data)
        : "memory"
        );
}
ILI9481_ERROR ILI9481::init() {


    // 0. 参数检查
    uint32_t ret;
    ret = (uint32_t)_delayMs;
    if (ret == 0) {
        return ILI9481_ERROR::PARAM_ERR;
    }
    // 1. 退出休眠
    __WRITE_REG(ILI9481_CMD::EXIT_SLEEP); // 退出休眠
    _delayMs(200);

    // 2. 电压配置
    __WRITE_REG(ILI9481_CMD::POWER_SETTING);
    for (uint8_t i = 0; i < sizeof(powerSettingParam); i++) {
        __WRITE_DATA(powerSettingParam[i]);
    }

    __WRITE_REG(ILI9481_CMD::VCOM_CONTROL);
    for (uint8_t i = 0; i < sizeof(vcomSettingParam); i++) {
        __WRITE_DATA(vcomSettingParam[i]);
    }

    __WRITE_REG(ILI9481_CMD::POWER_SETTING_FOR_NORMAL_MODE);
    for (uint8_t i = 0; i < sizeof(normalModePowerParam); i++) {
        __WRITE_DATA(normalModePowerParam[i]);
    }

    // 3. 显示设置
    __WRITE_REG(ILI9481_CMD::PANEL_DRIVING_SETTING);
    for (uint8_t i = 0; i < sizeof(panelDrivingParam); i++) {
        __WRITE_DATA(panelDrivingParam[i]);
    }

    // 4. 刷新帧率
    __WRITE_REG(ILI9481_CMD::FRAME_RATE_AND_INVERSION_CONTROL);
    for (uint8_t i = 0; i < sizeof(frameRateParame); i++) {
        __WRITE_DATA(frameRateParame[i]);
    }

    // 5. Gamma
    __WRITE_REG(ILI9481_CMD::GAMMA_SETTING);
    for (uint8_t i = 0; i < sizeof(gammaParam); i++) {
        __WRITE_DATA(gammaParam[i]);
    }

    // 6. 显示方向
    __WRITE_REG(ILI9481_CMD::SET_ADDR_MODE);
    __WRITE_DATA(ADDR_MODE_PARAM);

    // 7. 像素格式
    __WRITE_REG(ILI9481_CMD::SET_PIXEL_FORMAT);
    __WRITE_DATA(PIXEL_FORMAT);

    // 8. 列地址范围
    __WRITE_REG(ILI9481_CMD::SET_COL_ADDR);
    for (uint8_t i = 0; i < sizeof(columnAddr); i++) {
        __WRITE_DATA(columnAddr[i]);
    }

    // 9. 行地址范围
    __WRITE_REG(ILI9481_CMD::SET_PAGE_ADDR);
    for (uint8_t i = 0; i < sizeof(pageAddr); i++) {
        __WRITE_DATA(pageAddr[i]);
    }

    exec_time_us = TIM24->CNT;
    __WRITE_REG(ILI9481_CMD::WRITE_MEM_START)
    for (uint32_t i = 0; i < 320 * 480; i++) {

        __WRITE_DATA( i );

    }
    exec_time_us = TIM24->CNT - exec_time_us;

    // 10. 开始显示
    __WRITE_REG(ILI9481_CMD::SET_DISPLAY_ON);


    return ILI9481_ERROR::OK;
}


ILI9481_ERROR ILI9481::test() {

    _delayMs(1000);
    __WRITE_REG(ILI9481_CMD::ENTER_INVERT);
    _delayMs(1000);
    __WRITE_REG(ILI9481_CMD::EXIT_INVERT);

    return ILI9481_ERROR::OK;
}


ILI9481_ERROR ILI9481::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {

    // 越界检测（可根据你屏幕实际分辨率修改）
    if (w == 0 || h == 0)
        return ILI9481_ERROR::PARAM_ERR;
    if (x >= 320 || y >= 480)
        return ILI9481_ERROR::PARAM_ERR;

    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    if (x_end >= 320) x_end = 319;
    if (y_end >= 480) y_end = 479;

    // -----------------------------
    // 设置列地址 SET_COLUMN_ADDR
    // -----------------------------
    __WRITE_REG((uint16_t)ILI9481_CMD::SET_COL_ADDR);
    __WRITE_DATA(x >> 8);
    __WRITE_DATA(x & 0xFF);
    __WRITE_DATA(x_end >> 8);
    __WRITE_DATA(x_end & 0xFF);

    // -----------------------------
    // 设置页地址 SET_PAGE_ADDR
    // -----------------------------
    __WRITE_REG((uint16_t)ILI9481_CMD::SET_PAGE_ADDR);
    __WRITE_DATA(y >> 8);
    __WRITE_DATA(y & 0xFF);
    __WRITE_DATA(y_end >> 8);
    __WRITE_DATA(y_end & 0xFF);

    // -----------------------------
    // 开始写入像素数据 RAM WRITE
    // -----------------------------
    __WRITE_REG(ILI9481_CMD::WRITE_MEM_START);

    uint32_t total = (uint32_t)w * (uint32_t)h;

    while (total--)
        __WRITE_DATA(color);

    return ILI9481_ERROR::OK;
}

inline void ILI9481::drawPixel(uint16_t x, uint16_t y, uint16_t color) {

}