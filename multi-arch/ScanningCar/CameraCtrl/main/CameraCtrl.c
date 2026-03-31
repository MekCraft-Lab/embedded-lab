#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "injected/esp_wifi.h"
#include "injected/esp_wifi_types_generic.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"

#include <stdatomic.h>
#include <string.h>

// ================== 请在这里配置你的测试参数 ==================
#define TEST_WIFI_SSID      "TJURM"      // 你的路由器 Wi-Fi 名称
#define TEST_WIFI_PASS      "tjurm2020"  // 你的路由器 Wi-Fi 密码
#define DEST_SERVER_IP      "192.168.28.253"       // 运行 Python 接收端的电脑 IP
#define DEST_SERVER_PORT    5001                  // 目标端口
#define PAYLOAD_SIZE        1460                  // UDP MTU 优化大小
// ==============================================================

static const char *TAG = "SPI_STRESS_TEST";

// 事件标志
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

// 全局原子计数器，用于跨任务安全地统计发送字节数
volatile uint64_t total_bytes_sent = 0;

/* ------------------------------------------------------------------
 * 1. Wi-Fi 事件回调处理
 * ------------------------------------------------------------------ */
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi 驱动已启动，正在尝试连接 AP...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "连接断开，正在重试...");
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "🟢 成功连接路由器！获取到 IP 地址: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

/* ------------------------------------------------------------------
 * 2. 吞吐量实时监控任务 (每 100ms 更新一次)
 * ------------------------------------------------------------------ */
void throughput_monitor_task(void *pvParameters) {
    uint64_t last_bytes = 0;
    double mbps_sum = 0;
    int tick_count = 0;

    ESP_LOGI(TAG, "监控任务已启动，等待数据流...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100)); // 精确的 100ms 采样周期

        uint64_t current_bytes = __atomic_load_n(&total_bytes_sent, __ATOMIC_RELAXED);
        uint64_t diff_bytes = current_bytes - last_bytes;
        last_bytes = current_bytes;

        // 计算 100ms 内的即时速率 (Mbps)
        // 公式: (字节数 * 8位) / (0.1秒 * 1024 * 1024)
        double current_mbps = (diff_bytes * 8.0) / (0.1 * 1024 * 1024);

        // 只有当有数据流动时才打印，避免刷屏
        if (current_mbps > 0.1) {
            ESP_LOGI("PERF_100ms", "即时速率: %6.2f Mbps", current_mbps);

            mbps_sum += current_mbps;
            tick_count++;

            // 每 10 次 (即 1 秒) 打印一次平均速率
            if (tick_count >= 10) {
                ESP_LOGW("PERF_1Sec", "========== 平均速率: %6.2f Mbps ==========", mbps_sum / 10.0);
                mbps_sum = 0;
                tick_count = 0;
            }
        }
    }
}

/* ------------------------------------------------------------------
 * 3. UDP 狂暴发包任务 (极限压榨网络带宽)
 * ------------------------------------------------------------------ */
void udp_stress_test_task(void *pvParameters) {
    // 等待 Wi-Fi 连接成功
    ESP_LOGI(TAG, "发包任务挂起，等待网络连接...");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "网络就绪！开始向 %s:%d 倾泻 UDP 数据...", DEST_SERVER_IP, DEST_SERVER_PORT);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "无法创建套接字!");
        vTaskDelete(NULL);
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(DEST_SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_SERVER_PORT);

    // 构造一个固定大小的 dummy 数据包
    uint8_t *payload = malloc(PAYLOAD_SIZE);
    if (!payload) {
        ESP_LOGE(TAG, "内存分配失败!");
        close(sock);
        vTaskDelete(NULL);
    }
    // 填充一些测试特征码
    memset(payload, 0xAA, PAYLOAD_SIZE);
    payload[0] = 0xDE; payload[1] = 0xAD;
    payload[PAYLOAD_SIZE-2] = 0xBE; payload[PAYLOAD_SIZE-1] = 0xEF;

    // 死循环全速发送
    while (1) {
        int err = sendto(sock, payload, PAYLOAD_SIZE, 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err > 0) {
            // 安全累加已发送的字节数，供监控任务读取
            atomic_fetch_add(&total_bytes_sent, err);
        } else {
            // 如果底层缓冲区满了发不出去，稍微让出一下 CPU
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

/* ------------------------------------------------------------------
 * 4. 主程序入口
 * ------------------------------------------------------------------ */
void app_main(void) {
    ESP_LOGI(TAG, "ESP32-P4 + C6 SPI 全双工链路极限压测启动...");

    // 基础组件初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_event_group = xEventGroupCreate();

    // 注册事件监听器
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    // 配置由 ESP-Hosted 托管的 Wi-Fi 驱动
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TEST_WIFI_SSID,
            .password = TEST_WIFI_PASS,
            // 如果你知道路由器的频段，强制设置可以加快连接速度
            // .channel = 1,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 创建压测相关任务
    // 监控任务优先级高一些，确保准时打印
    xTaskCreatePinnedToCore(throughput_monitor_task, "monitor_task", 4096, NULL, 5, NULL, 0);

    // 发包任务优先级略低，放在另一个核心或者同一个核心疯狂压榨 CPU
    xTaskCreatePinnedToCore(udp_stress_test_task, "udp_tx_task", 4096, NULL, 4, NULL, 1);
}