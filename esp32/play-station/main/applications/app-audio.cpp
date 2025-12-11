/**
 *******************************************************************************
 * @file    app-audio.cpp
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

#define I2S_NUM           I2S_NUM_0
#define I2S_BCK_IO        44
#define I2S_WS_IO         45
#define I2S_DO_IO         46

#define I2S_DMA_BUF_LEN   512

#define I2S_DMA_BUF_COUNT 8

#define SAMPLE_RATE       44100


/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-audio.h"

#include <cmath>

/* II. other application */


/* III. standard lib */




/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

#define BUF_SAMPLES 512
int16_t buf[BUF_SAMPLES * 2]; // 立体声缓冲



/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Audio"

#define APPLICATION_STACK_SIZE 128

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];





/* ------- message interface attribute -------------------------------------------------------------------------------*/





/* ------- function prototypes ---------------------------------------------------------------------------------------*/

static void generate_note(int16_t*, size_t, float);





/* ------- function implement ----------------------------------------------------------------------------------------*/


AudioApp::AudioApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 0,
                    nullptr) {}


AudioApp& AudioApp::instance() {

    static AudioApp audioApp;
    return audioApp;
}


void AudioApp::init() {
    /* driver object initialize */

    i2s_config_t i2s_config = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = SAMPLE_RATE,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = 0,
        .dma_buf_count        = I2S_DMA_BUF_COUNT,
        .dma_buf_len          = I2S_DMA_BUF_LEN,
        .use_apll             = true,
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO, .ws_io_num = I2S_WS_IO, .data_out_num = I2S_DO_IO, .data_in_num = -1};

    // 将配置注册到驱动中
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
}


void AudioApp::run() {
    float notes[]     = {440.0, 494.0, 523.3, 587.3}; // A4, B4, C5, D5
    size_t note_index = 0;

    while (1) {
        generate_note(buf, BUF_SAMPLES, notes[note_index]);
        size_t bytes_written;
        i2s_write(I2S_NUM, buf, sizeof(buf), &bytes_written, portMAX_DELAY);

        note_index = (note_index + 1) % (sizeof(notes) / sizeof(notes[0]));
    }
}



uint8_t AudioApp::rxMsg(void* msg, uint16_t size) { return 0; }

uint8_t AudioApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) { return 0; }


// 生成一个正弦波音符
static void generate_note(int16_t* buf, size_t samples, float frequency) {
    for (size_t i = 0; i < samples; i++) {
        int16_t sample = (int16_t)(sin(2 * M_PI * frequency * i / SAMPLE_RATE) * 30000);
        buf[i * 2 + 0] = sample; // 左声道
        buf[i * 2 + 1] = sample; // 右声道
    }
}