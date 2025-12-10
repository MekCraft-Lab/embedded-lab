#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

static const char *TAG = "wifi_remote_check";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA started");
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connected to AP — 说明：P4 <-> C6 通信正常！");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "Disconnected from AP");
                break;

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        ESP_LOGI(TAG, "🎉 Wi-Fi 完全正常 —— 与 C6 通信成功！");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "===== WiFi Remote / C6 协处理器验证程序 =====");

    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return;
    }

    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());

    // 事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建 STA 接口
    esp_netif_create_default_wifi_sta();

    // WiFi 配置
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_LOGI(TAG, "Initializing esp_wifi...");
    ret = esp_wifi_init(&cfg);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "⚠️ esp_wifi_init() 失败：%s\n"
                 "这通常表示：P4 无法与 C6 协处理器通信！",
                 esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "esp_wifi_init 成功 —— 初步判断 P4 <-> C6 通信建立");

    // 注册事件
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL)
    );
    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL)
    );

    // WiFi 模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 设置 WiFi STA 信息（改成你自己的）
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "TJURM",
            .password = "tjurm2020",
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_LOGI(TAG, "启动 WiFi...");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "等待连接结果...");
}
