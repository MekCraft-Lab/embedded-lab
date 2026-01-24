#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "esp_random.h"

#include "driver/gpio.h"
#include "led_strip.h"

/* ================= GPIO 定义 ================= */
#define WS2812_PIN (gpio_num_t)11

#define CS0_PIN    (gpio_num_t)14
#define CS1_PIN    (gpio_num_t)21
#define CS2_PIN    (gpio_num_t)38


static const gpio_num_t ring_key_pins[10] = {(gpio_num_t)1, (gpio_num_t)2, (gpio_num_t)4, (gpio_num_t)5,
                                             (gpio_num_t)6, (gpio_num_t)7, (gpio_num_t)10,  (gpio_num_t)12,
                                             (gpio_num_t)8, (gpio_num_t)9};

/* ================= LED 参数 ================= */
#define REGION1_LED_NUM  301
#define REGION23_LED_NUM 100

static const uint16_t ring_led_count[10] = {40, 40, 42, 36, 36, 20, 20, 12, 8, 4};

/* ================= 全局对象 ================= */
static led_strip_handle_t led_strip;
static SemaphoreHandle_t ws2812_mutex;
static QueueHandle_t key_evt_queue;

/* ================= 4051 选择 ================= */
static inline void select_4051(uint8_t ch) {
    gpio_set_level(CS0_PIN, ch & 0x01);
    gpio_set_level(CS1_PIN, (ch >> 1) & 0x01);
    gpio_set_level(CS2_PIN, (ch >> 2) & 0x01);
}

/* ================= WS2812 初始化 ================= */
static void ws2812_init() {
    led_strip_config_t strip_config = {
        .strip_gpio_num         = WS2812_PIN,
        .max_leds               = REGION1_LED_NUM, // 最大区域
        .led_model              = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma    = false,
        },
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    led_strip_clear(led_strip);
}

/* ================= 刷灯工具函数 ================= */
static void ws2812_show_buf(uint16_t led_num, uint8_t* buf) {
    for (int i = 0; i < led_num; i++) {
        led_strip_set_pixel(led_strip, i,
                            buf[i * 3 + 1], // R
                            buf[i * 3 + 0], // G
                            buf[i * 3 + 2]  // B
        );
    }
    led_strip_refresh(led_strip);
}

/* ================= 按键中断 ================= */
typedef struct {
    uint32_t ring_id;
} key_event_t;

static void IRAM_ATTR key_isr(void* arg) {
    uint32_t ring   = (uint32_t)arg;
    key_event_t evt = {.ring_id = ring};
    xQueueSendFromISR(key_evt_queue, &evt, NULL);
}

/* ================= 区域1任务 ================= */

typedef struct {
    bool        active;        // 是否在亮
    TickType_t start_tick;     // 触发时间
    uint8_t    r, g, b;        // 初始颜色
} ring_state_t;

static ring_state_t rings[10];

#define RING_FADE_TIME_MS   1000    // 总衰减时间
#define REGION1_REFRESH_MS   20     // 刷新周期（50 FPS）

static void task_region1(void *arg)
{
    uint8_t buf[REGION1_LED_NUM * 3];
    key_event_t evt;

    uint16_t offset[10];
    offset[0] = 0;
    for (int i = 1; i < 10; i++) {
        offset[i] = offset[i - 1] + ring_led_count[i - 1];
    }

    memset(rings, 0, sizeof(rings));

    const TickType_t fade_ticks =
        pdMS_TO_TICKS(RING_FADE_TIME_MS);

    while (1) {

        /* ---------- 处理所有按键事件（非阻塞） ---------- */
        while (xQueueReceive(key_evt_queue, &evt, 0) == pdTRUE) {
            ring_state_t *ring = &rings[evt.ring_id];

            ring->active     = true;
            ring->start_tick = xTaskGetTickCount();
            ring->r = esp_random() & 0xFF;
            ring->g = esp_random() & 0xFF;
            ring->b = esp_random() & 0xFF;
        }

        /* ---------- 生成当前帧 ---------- */
        memset(buf, 0, sizeof(buf));
        TickType_t now = xTaskGetTickCount();

        for (int ring_id = 0; ring_id < 10; ring_id++) {
            ring_state_t *ring = &rings[ring_id];

            if (!ring->active)
                continue;

            TickType_t elapsed = now - ring->start_tick;

            if (elapsed >= fade_ticks) {
                ring->active = false;
                continue;
            }

            /* 线性衰减（你可换成 gamma / exp） */
            float k = 1.0f - (float)elapsed / fade_ticks;

            uint8_t r = (uint8_t)(ring->r * k);
            uint8_t g = (uint8_t)(ring->g * k);
            uint8_t b = (uint8_t)(ring->b * k);

            for (int i = 0; i < ring_led_count[ring_id]; i++) {
                int idx = offset[ring_id] + i;
                buf[idx * 3 + 0] = g;
                buf[idx * 3 + 1] = r;
                buf[idx * 3 + 2] = b;
            }
        }

        /* ---------- 刷灯 ---------- */
        xSemaphoreTake(ws2812_mutex, portMAX_DELAY);
        select_4051(0);  // Y0
        ws2812_show_buf(REGION1_LED_NUM, buf);
        xSemaphoreGive(ws2812_mutex);

        vTaskDelay(pdMS_TO_TICKS(REGION1_REFRESH_MS));
    }
}

/* ================= 呼吸灯任务 ================= */
static void task_breath(void* arg) {
    uint32_t param  = (uint32_t)arg;
    uint8_t channel = (param >> 16) & 0xFF;
    uint16_t period = param & 0xFFFF;

    uint8_t buf[REGION23_LED_NUM * 3];

    while (1) {
        for (int i = 0; i <= 50; i++) {
            uint8_t v = (uint8_t)(50.0f * i / 50);
            for (int j = 0; j < REGION23_LED_NUM; j++) {
                buf[j * 3 + 0] = v;
                buf[j * 3 + 1] = v;
                buf[j * 3 + 2] = v;
            }

            xSemaphoreTake(ws2812_mutex, portMAX_DELAY);
            select_4051(channel);
            ws2812_show_buf(REGION23_LED_NUM, buf);
            xSemaphoreGive(ws2812_mutex);

            vTaskDelay(period / 100);
        }

        for (int i = 50; i >= 0; i--) {
            uint8_t v = (uint8_t)(50.0f * i / 50);
            for (int j = 0; j < REGION23_LED_NUM; j++) {
                buf[j * 3 + 0] = v;
                buf[j * 3 + 1] = v;
                buf[j * 3 + 2] = v;
            }

            xSemaphoreTake(ws2812_mutex, portMAX_DELAY);
            select_4051(channel);
            ws2812_show_buf(REGION23_LED_NUM, buf);
            xSemaphoreGive(ws2812_mutex);

            vTaskDelay(period / 100);
        }
    }
}

/* ================= app_main ================= */
uint8_t test_buf[50 * 3] = {0};

extern "C" void app_main(void) {
    /* 4051 GPIO */
    gpio_config_t out_cfg = {
        .pin_bit_mask = (1ULL << CS0_PIN) | (1ULL << CS1_PIN) | (1ULL << CS2_PIN) | (1ULL << 13),

        .mode         = GPIO_MODE_OUTPUT,

    };
    gpio_config(&out_cfg);

    /* 按键 GPIO */
    gpio_config_t in_cfg = {
        .mode       = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type  = GPIO_INTR_NEGEDGE,
    };

    gpio_install_isr_service(0);

    for (int i = 0; i < 10; i++) {
        in_cfg.pin_bit_mask = 1ULL << ring_key_pins[i];
        gpio_config(&in_cfg);
        gpio_isr_handler_add(ring_key_pins[i], key_isr, (void*)i);
    }

    ws2812_init();

    ws2812_mutex  = xSemaphoreCreateMutex();
    key_evt_queue = xQueueCreate(10, sizeof(key_event_t));

    for (uint8_t i = 0; i < 50; i++) {
        test_buf[i * 3 + 0] = 20;
        test_buf[i * 3 + 1] = 10;
        test_buf[i * 3 + 2] = 38;
    }
    printf("开始显示\n");
    gpio_set_level((gpio_num_t)13, 0);



    xTaskCreate(task_region1, "region1", 4096, NULL, 5, NULL);

    xTaskCreate(task_breath, "region2", 4096, (void*)((1 << 16) | 845), 3, NULL);

    xTaskCreate(task_breath, "region3", 4096, (void*)((2 << 16) | 687), 3, NULL);

    vTaskDelete(NULL);
}
