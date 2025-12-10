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

static void lcdPanelInit(esp_lcd_panel_io_handle_t, esp_lcd_panel_handle_t);


/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

static esp_lcd_i80_bus_handle_t i80Bus;

static esp_lcd_panel_io_handle_t ioHandle;

static esp_lcd_panel_handle_t panelHandle;

/* ------- function implement ----------------------------------------------------------------------------------------*/


/**
 * @brief 创建8080总线
 */
void lcdInitBusIOAndPanel(esp_lcd_i80_bus_handle_t* pBusHandle, esp_lcd_panel_io_handle_t* pIoHandle, esp_lcd_panel_handle_t* pPanelHandle) {
    esp_lcd_i80_bus_config_t busConfig = {
        .dc_gpio_num        = DC_PIN,
        .wr_gpio_num        = WRITE_ENABLE_PIN,
        .clk_src            = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums     = {D0_PIN, D1_PIN, D2_PIN, D3_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN}, // D0~D7
        .bus_width          = 8,
        .max_transfer_bytes = 240 * 40 * 2, // 每次 DMA 最大传输
    };
    esp_lcd_new_i80_bus(&busConfig, &i80Bus);

    lcdPanelIOInit(i80Bus, &ioHandle);

    lcdPanelInit(ioHandle, panelHandle);

    *pBusHandle = i80Bus;

    *pIoHandle  = ioHandle;

    *pPanelHandle = panelHandle;
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
    };

    esp_lcd_new_panel_io_i80(busHandle, &ioConfig, ioHandle_);
}

/**
 * @brief 初始化panelHandle
 * @param ioHandle_ IO句柄
 * @param panelHandle_ 屏幕句柄
 */
static void lcdPanelInit(esp_lcd_panel_io_handle_t ioHandle_, esp_lcd_panel_handle_t panelHandle_) {
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = RESET_PIN,
        .rgb_endian     = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16, // RGB565
    };


    esp_lcd_new_panel_st7789(ioHandle_, &panel_config, &panelHandle_);

    // 初始化 LCD
    esp_lcd_panel_reset(panelHandle_);
    esp_lcd_panel_init(panelHandle_);
    esp_lcd_panel_invert_color(panelHandle_, true);
}