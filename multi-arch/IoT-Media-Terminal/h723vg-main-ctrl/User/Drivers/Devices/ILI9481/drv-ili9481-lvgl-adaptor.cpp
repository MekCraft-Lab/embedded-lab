/**
 *******************************************************************************
 * @file    drv-ili9481-lvgl-adaptor.cpp
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
 * @date    2025/11/30
 * @version 1.0
 *******************************************************************************
 */


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "drv-ili9481-lvgl-adaptor.h"
#include "drv-ili9481.h"




/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


void lcdInit() {
    ILI9481::init();
}

void lcdFillRect(uint16_t x, uint16_t y, uint16_t endx, uint16_t endy, uint16_t* color) {

    // 越界检测（可根据你屏幕实际分辨率修改）

    uint16_t w = endx - x + 1;
    uint16_t h = endy - y + 1;

    if (w >= 480) w = 479;
    if (h >= 320) h = 319;

    // -----------------------------
    // 设置页地址 SET_PAGE_ADDR
    // -----------------------------
    __WRITE_REG((uint16_t)ILI9481_CMD::SET_PAGE_ADDR);
    __WRITE_DATA(x >> 8);
    __WRITE_DATA(x & 0xFF);
    __WRITE_DATA(endx>> 8);
    __WRITE_DATA(endx& 0xFF);

    // -----------------------------
    // 设置列地址 SET_COLUMN_ADDR
    // -----------------------------
    __WRITE_REG((uint16_t)ILI9481_CMD::SET_COL_ADDR);
    __WRITE_DATA(y >> 8);
    __WRITE_DATA(y & 0xFF);
    __WRITE_DATA(endy >> 8);
    __WRITE_DATA(endy & 0xFF);

    // -----------------------------
    // 开始写入像素数据 RAM WRITE
    // -----------------------------


    __WRITE_REG(ILI9481_CMD::WRITE_MEM_START);


    uint32_t total = w * h;

    for (uint16_t i = 0; i < w; i++) {
        for (uint16_t j = 0; j < h; j++) {
            __WRITE_DATA(color[j * w + i])
        }
    }

}
void lcdFillRectScale2(uint16_t x, uint16_t y, uint16_t endx, uint16_t endy, uint16_t* color) {

    // 放大后的坐标
    uint16_t new_x = x * 2;
    uint16_t new_y = y * 2;
    uint16_t new_endx = (endx + 1) * 2 - 1; // 保持宽度一致
    uint16_t new_endy = (endy + 1) * 2 - 1; // 保持高度一致

    // 越界检测
    if (new_endx >= 480) new_endx = 479;
    if (new_endy >= 320) new_endy = 319;

    uint16_t w = new_endx - new_x + 1;
    uint16_t h = new_endy - new_y + 1;

    // -----------------------------
    // 设置页地址 SET_PAGE_ADDR
    // -----------------------------
    __WRITE_REG((uint16_t)ILI9481_CMD::SET_PAGE_ADDR);
    __WRITE_DATA(new_x >> 8);
    __WRITE_DATA(new_x & 0xFF);
    __WRITE_DATA(new_endx >> 8);
    __WRITE_DATA(new_endx & 0xFF);

    // -----------------------------
    // 设置列地址 SET_COLUMN_ADDR
    // -----------------------------
    __WRITE_REG((uint16_t)ILI9481_CMD::SET_COL_ADDR);
    __WRITE_DATA(new_y >> 8);
    __WRITE_DATA(new_y & 0xFF);
    __WRITE_DATA(new_endy >> 8);
    __WRITE_DATA(new_endy & 0xFF);

    // -----------------------------
    // 开始写入像素数据 RAM WRITE
    // -----------------------------
    __WRITE_REG(ILI9481_CMD::WRITE_MEM_START);


    for (uint16_t i = 0; i < w; i++) {
        for (uint16_t j = 0; j < h; j++) {
            __WRITE_DATA(color[(j/2) * (w/2) + (i/2)])
        }
    }

}
