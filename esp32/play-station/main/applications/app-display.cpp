/**
 *******************************************************************************
 * @file    app-display.cpp
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
 * @date    2025/12/12
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/

#define DISP_WIDTH      240
#define DISP_HEIGHT     240

#define NES_WIDTH       256
#define NES_HEIGHT      240


#define GUI_FIRSTENTRY  192
#define GUI_LASTENTRY   206
#define GUI_TOTALCOLORS (GUI_LASTENTRY - GUI_FIRSTENTRY)




/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-display.h"

#include "benchmark/lv_demo_benchmark.h"
#include "bitmap.h"
#include "esp_log.h"

#include <cstring>

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/


esp_lcd_i80_bus_handle_t i80Bus;

esp_lcd_panel_io_handle_t ioHandle;

esp_lcd_panel_handle_t panelHandle;

uint8_t* imageBuffer;

bitmap_t* bitmap;

static uint16_t paletteArr[256];


static const rgb_t gui_palette[GUI_TOTALCOLORS] = {
    {0x00, 0x00, 0x00}, // GUI_BLACK
    {0x40, 0x40, 0x40}, // GUI_DKGRAY
    {0x80, 0x80, 0x80}, // GUI_GRAY
    {0xC0, 0xC0, 0xC0}, // GUI_LTGRAY
    {0xFF, 0xFF, 0xFF}, // GUI_WHITE
    {0xFF, 0x00, 0x00}, // GUI_RED
    {0x00, 0xFF, 0x00}, // GUI_GREEN
    {0x00, 0x00, 0xFF}, // GUI_BLUE
    {0xFF, 0xFF, 0x00}, // GUI_YELLOW
    {0xFF, 0xA5, 0x00}, // GUI_ORANGE
    {0x80, 0x00, 0x80}, // GUI_PURPLE
    {0x00, 0x80, 0x80}, // GUI_TEAL
    {0x00, 0x64, 0x00}, // GUI_DKGREEN
    {0x00, 0x00, 0x8B}, // GUI_DKBLUE
};



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Dispaly"

#define APPLICATION_STACK_SIZE 4096

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];





/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/


DisplayApp::DisplayApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


DisplayApp& DisplayApp::instance() {
    static DisplayApp displayApp;
    return displayApp;
}


void DisplayApp::init() {

    /* LCD IO */
    lcdInitBusIOAndPanel(&i80Bus, &ioHandle, &panelHandle);

    esp_lcd_panel_reset(panelHandle);

    st7789Init();
    _nofrendoUpdate = xSemaphoreCreateBinary();


    gpio_reset_pin(GPIO_NUM_17);
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);

#ifdef LVGL_TURN_ON
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();

    esp_err_t err                  = lvgl_port_init(&lvgl_cfg);


    static lv_disp_t* disp_handle;


    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {.io_handle     = ioHandle,
                                              .panel_handle  = panelHandle,
                                              .buffer_size   = DISP_WIDTH * 20 * 2,
                                              .double_buffer = true,
                                              .hres          = DISP_WIDTH,
                                              .vres          = DISP_HEIGHT,
                                              .monochrome    = false,
                                              .rotation =
                                                  {
                                                      .swap_xy  = false,
                                                      .mirror_x = false,
                                                      .mirror_y = false,
                                                  },
                                              .color_format = LV_COLOR_FORMAT_RGB565,
                                              .flags        = {
                                                         .buff_dma   = true,
                                                         .swap_bytes = true,
                                              }};

    disp_handle                            = lvgl_port_add_disp(&disp_cfg);

    /* ... the rest of the initialization ... */


    if (lvgl_port_lock(30)) {

        lv_demo_benchmark();

        lvgl_port_unlock();
    }

    vTaskDelete(nullptr);

#endif
}


#define SCREEN_HEIGHT 240

#define BLOCK_HEIGHT  20 // 每次刷新 20 行
#define NUM_BLOCKS    (SCREEN_HEIGHT / BLOCK_HEIGHT)
#define BLOCK_HEIGHT  20
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240
#define BITMAP_WIDTH  256
static uint16_t frameBuffer[240 * 240]; // 20*240 RGB565

uint32_t fps;
void DisplayApp::run() {

    if (xSemaphoreTake(_nofrendoUpdate, portMAX_DELAY) == pdPASS) {
        static uint8_t i;
        gpio_set_level(GPIO_NUM_17, i++%2); // 初始低电平
            esp_lcd_panel_draw_bitmap(panelHandle, 0, 0, 240, 240, frameBuffer);

    }
}



uint8_t DisplayApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t DisplayApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


/*------------ 提供驱动 --------------*/


void playSound(void* buf, int size) {
    (void)buf;
    (void)size;
}




int vidInit(int width, int height) {

    (void)width;
    (void)height;

    ESP_LOGI("DispalyApp", "Alloc memory");

    if ((imageBuffer = (uint8_t*)malloc(NES_WIDTH * NES_HEIGHT * sizeof(uint8_t))) != nullptr) {
        ESP_LOGI("DisplayApp", "imageBuffer指向的地址:%p", imageBuffer);
        return 0;
    }

    return -1;
}

void vidShutdown() {}

int vidSetMode(int width, int height) {
    (void)width;
    (void)height;
    return 0;
}



static inline uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}


// NES 调色板 + GUI 调色板同时初始化
void vidSetPalette(rgb_t* nes_palette_64) {
    // 1. 先写入 NES 的 64 色（放在 paletteArr[0~63]）
    for (int i = 0; i < 64; i++) {
        paletteArr[i] = rgb_to_565(nes_palette_64[i].r, nes_palette_64[i].g, nes_palette_64[i].b);
    }

    // 2. GUI 颜色必须写入 paletteArr[192~205]
    for (int i = 0; i < GUI_TOTALCOLORS; i++) {
        int idx         = GUI_FIRSTENTRY + i;
        paletteArr[idx] = rgb_to_565(gui_palette[i].r, gui_palette[i].g, gui_palette[i].b);
    }

    // 其余 paletteArr[64~191] 由模拟器自己填充，不动
}


void vidClear(uint8 color) { memset(imageBuffer, paletteArr[color], NES_HEIGHT * NES_WIDTH); }



bitmap_t* vidLockWrite() {
    bitmap = bmp_create(NES_WIDTH, NES_HEIGHT, NES_WIDTH * 2);
    return bitmap;
}



/* release the resource */
void vidFreeWrite(int num_dirties, rect_t* dirty_rects) {
    (void)num_dirties;
    (void)dirty_rects;
    bmp_destroy(&bitmap);
}

#define SRC_W 256
#define SRC_H 262
#define DST_W 240
#define DST_H 240

void resize_and_palette(uint8_t *src, uint16_t *dst, const uint16_t *palette)
{
    for (int j = 0; j < DST_H; j++) {
        int src_y = j * SRC_H / DST_H;
        for (int i = 0; i < DST_W; i++) {
            int src_x = i * SRC_W / DST_W;

            // 取原 bitmap 索引
            uint8_t pixel_index = src[src_y * SRC_W + src_x];

            // 直接查表得到 RGB565
            dst[j * DST_W + i] = palette[pixel_index];
        }
    }
}

void vidBlit(bitmap_t* primary, int num_dirties, rect_t* dirty_rects) {

    resize_and_palette(primary->line[0], frameBuffer, paletteArr);

    (void)num_dirties;
    (void)dirty_rects;
    fps++;
    xSemaphoreGive(DisplayApp::instance()._nofrendoUpdate);
}

int logout(const char* msg) {

    ESP_LOGI("NES-LOG", "%s", msg);

    return 0;
}
