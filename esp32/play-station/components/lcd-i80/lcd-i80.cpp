/**
 *******************************************************************************
 * @file    lcd-i80.cpp
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
 * @date    2025/12/11
 * @version 1.0
 *******************************************************************************
 */


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "lcd-i80.h"

#include "esp_lcd_panel_ops.h"



/* ------- class prototypes-------------------------------------------------------------------------------------------*/

static void lcdPanelIOInit(esp_lcd_i80_bus_handle_t, esp_lcd_panel_io_handle_t*);

static void lcdPanelInit(esp_lcd_panel_io_handle_t, esp_lcd_panel_handle_t*);


/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static esp_lcd_panel_io_handle_t* _pIoHandle;


/* ------- function implement ----------------------------------------------------------------------------------------*/


/**
 * @brief 创建8080总线
 */
extern "C" void lcdInitBusIOAndPanel(esp_lcd_i80_bus_handle_t* pBusHandle, esp_lcd_panel_io_handle_t* pIoHandle, esp_lcd_panel_handle_t* pPanelHandle) {
    esp_lcd_i80_bus_config_t busConfig = {
        .dc_gpio_num        = DC_PIN,
        .wr_gpio_num        = WRITE_ENABLE_PIN,
        .clk_src            = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums     = {D0_PIN, D1_PIN, D2_PIN, D3_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN}, // D0~D7
        .bus_width          = 8,
        .max_transfer_bytes = 240 * 40 * 2, // 每次 DMA 最大传输
    };
    esp_lcd_new_i80_bus(&busConfig, pBusHandle);

    lcdPanelIOInit(*pBusHandle, pIoHandle);

    lcdPanelInit(*pIoHandle, pPanelHandle);

    _pIoHandle = pIoHandle;

}


/**
 * @brief 创建Panel IO
 * @param busHandle 总线句柄
 * @param ioHandle_ IO句柄
 */
static void lcdPanelIOInit(esp_lcd_i80_bus_handle_t busHandle, esp_lcd_panel_io_handle_t* ioHandle_) {
    esp_lcd_panel_io_i80_config_t ioConfig = {
        .cs_gpio_num         = CS_PIN,
        .pclk_hz             = COMM_FREQ,
        .trans_queue_depth   = 10,
        .on_color_trans_done = nullptr,
        .user_ctx            = nullptr,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .dc_levels = {
            .dc_cmd_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .cs_active_high = 0,
        },
    };

    esp_lcd_new_panel_io_i80(busHandle, &ioConfig, ioHandle_);
}

/**
 * @brief 初始化panelHandle
 * @param ioHandle_ IO句柄
 * @param pPanelHandle 屏幕句柄
 */
static void lcdPanelInit(esp_lcd_panel_io_handle_t ioHandle_, esp_lcd_panel_handle_t* pPanelHandle) {
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = RESET_PIN,
        .rgb_endian     = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16, // RGB565
    };


    esp_lcd_new_panel_st7789(ioHandle_, &panel_config, pPanelHandle);

    // 初始化 LCD
    // esp_lcd_panel_reset(*pPanelHandle);
    // esp_lcd_panel_init(*pPanelHandle);
    // esp_lcd_panel_invert_color(*pPanelHandle, true);
}

extern "C" void st7789Init() {

    vTaskDelay(100);


    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << READ_ENABLE_PIN,
        .mode = GPIO_MODE_OUTPUT,          // 推挽输出
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_set_level(static_cast<gpio_num_t>(READ_ENABLE_PIN), 1);
    gpio_config(&io_conf);


    esp_lcd_panel_io_tx_param(*_pIoHandle, 0x36, (uint8_t[]){0x00}, 1);


    esp_lcd_panel_io_tx_param(*_pIoHandle, 0x3A, (uint8_t[]){0x05}, 1);


    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xB2, (uint8_t[]){0x0C, 0x0C, 0x00, 0x33, 0x33}, 5);


    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xB7, (uint8_t[]){0x35}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xBB, (uint8_t[]){0x19}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xC0, (uint8_t[]){0x2C}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xC2, (uint8_t[]){0x01}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xC3, (uint8_t[]){0x12}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xC4, (uint8_t[]){0x20}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xC6, (uint8_t[]){0x0F}, 1);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xD0, (uint8_t[]){0xA4, 0xA1}, 2);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xE0, (uint8_t[]){0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23}, 14);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0xE1, (uint8_t[]){0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23}, 14);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0x21, nullptr, 0);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0x11, nullptr, 0);

    esp_lcd_panel_io_tx_param(*_pIoHandle, 0x29, nullptr, 0);


}