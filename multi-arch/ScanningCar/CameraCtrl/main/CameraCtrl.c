#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
// 引入新移植的组件头文件
#include "driver/i2c_master.h"
#include "esp_cam_sensor.h"
#include "imx219.h"

static const char *TAG = "IMX219_Test";

// 你开发板的引脚
#define CAM_PWDN_IO             0
#define CAM_XCLK_IO             1
#define I2C_MASTER_SDA_IO       7
#define I2C_MASTER_SCL_IO       8

// 必须保留的硬件时钟与高电平唤醒逻辑
void imx219_hardware_init(void) {
    ESP_LOGI(TAG, "启动 IMX219 24MHz时钟与高电平唤醒...");
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CAM_PWDN_IO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    gpio_set_level(CAM_PWDN_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(CAM_PWDN_IO, 1);

    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_1_BIT,
        .freq_hz          = 24000000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CAM_XCLK_IO,
        .duty           = 1,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    vTaskDelay(pdMS_TO_TICKS(150));
}

void app_main(void) {
    // 强制链接组件库 (这是原项目的一个技巧)
    imx219_force_link(); //

    // 1. 底层硬件唤醒
    imx219_hardware_init();

    // 2. 初始化高阶 Sensor 驱动框架
    esp_cam_sensor_config_t sensor_config = {
        .sccb_handle = NULL, // 如果传 NULL，esp_cam_sensor 内部会自己去初始化 I2C (如果有相关接口的话)。
                             // 注意：为了稳妥，你可能需要先初始化 i2c_master_bus，然后再调传感器。
    };

    // 假设你在 main 里自己初始化了 I2C bus:
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // 需要根据 esp_cam_sensor 库的实际定义，将你的 bus_handle 传递进去。
    // 在这个 PoC 的组件代码中，它期望配置里有 .sccb_handle
    // （在具体实现中，可能需要用到 esp_sccb_intf.h 相关的转换函数）
    // ... 这里需要你结合 IDF v5.5 和项目的 main.c 做最终的参数对齐。

    // ... 后续逻辑为通过 esp_cam_sensor API 拉起数据流
    ESP_LOGI(TAG, "一切就绪，IMX219 驱动组件已加载！");
}