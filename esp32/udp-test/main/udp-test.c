#include "mdns.h"
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define UDP_PORT 12345
#define BLOCK_SIZE 1500      // 每块数据大小
#define QUEUE_LENGTH 8       // 队列长度
#define UDP_BUF_SIZE BLOCK_SIZE

QueueHandle_t spi_queue;
#include "driver/spi_master.h"

#define SPI_DMA_CHAN SPI_DMA_CH_AUTO
#define SPI_CLK_HZ 8000000  // 1 MHz，可根据需求调整

spi_device_handle_t spi_handle;

void spi_master_init(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,  // 如果不接收可设 -1
        .mosi_io_num = 23,  // 根据你的 ESP32 接线修改
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BLOCK_SIZE,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CHAN);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLK_HZ,
        .mode = 0,
        .spics_io_num = 5,   // CS 引脚
        .queue_size = 3,
    };

    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
}
//----------------------- Wi-Fi STA -----------------------
void wifi_init_sta(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Ciallo～(∠・ω< )⌒★",
            .password = "0d000721",
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    esp_wifi_connect();
}

//----------------------- mDNS UDP 服务 -----------------------
void start_mdns_service(void)
{
    mdns_init();
    mdns_hostname_set("esp32c6");
    mdns_instance_name_set("ESP32C6 Video Device");
    mdns_service_add("ESP32C6 Video Stream", "_esp_video", "_udp", UDP_PORT, NULL, 0);
    mdns_service_txt_item_set("_esp_video", "_udp", "format", "rgb565");
    mdns_service_txt_item_set("_esp_video", "_udp", "resolution", "240x160");
    mdns_service_txt_item_set("_esp_video", "_udp", "fps", "10");
}

//----------------------- IP 事件 -----------------------
static void handler(void* arg, esp_event_base_t event_base,
                    int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("Got IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    }
}

//----------------------- SPI 发送任务 -----------------------
void spi_send_task(void *pvParameters)
{
    uint8_t* block;
    spi_master_init();
    while (1) {
        if (xQueueReceive(spi_queue, &block, portMAX_DELAY)) {
            spi_transaction_t t;
            memset(&t, 0, sizeof(t));
            t.length = BLOCK_SIZE * 8; // bit 数
            t.tx_buffer = block;

            // 阻塞发送（会自动用 DMA，如果 buffer 是 DMA 兼容的）
            esp_err_t ret = spi_device_transmit(spi_handle, &t);
            if (ret != ESP_OK) {
                printf("SPI transmit failed: %d\n", ret);
            }

            free(block);
        }
    }
}


//----------------------- UDP 接收任务 -----------------------
void udp_receive_task(void *pvParameters)
{
    int sock;
    struct sockaddr_in server_addr, client_addr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        printf("Failed to create socket\n");
        vTaskDelete(NULL);
        return;
    }

    int rcvbuf = 2048; // 增大接收缓冲区
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Socket bind failed\n");
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    printf("UDP server listening on port %d\n", UDP_PORT);

    while (1) {
        socklen_t addr_len = sizeof(client_addr);
        uint8_t rx_buffer[UDP_BUF_SIZE];

        int len = recvfrom(sock, rx_buffer, UDP_BUF_SIZE, 0,
                           (struct sockaddr *)&client_addr, &addr_len);
        if (len >= 4) {
            // 提取坐标
            uint16_t x = (rx_buffer[0] << 8) | rx_buffer[1];
            uint16_t y = (rx_buffer[2] << 8) | rx_buffer[3];
            // printf("Received UDP packet, length=%d, X=%d, Y=%d\n", len, x, y);

            if (x <= 220 && y <= 120) {
                // 分配动态内存存放块
                uint8_t* block = malloc(len);
                if (block) {
                    memcpy(block, rx_buffer, len);
                    // 放入 SPI 发送队列
                    if (xQueueSend(spi_queue, &block, 0) != pdPASS) {
                        // 队列满，丢弃
                        free(block);
                    }
                }
            }

        }
    }

    close(sock);
    vTaskDelete(NULL);
}

//----------------------- 主函数 -----------------------
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler, NULL);

    wifi_init_sta();
    start_mdns_service();

    // 创建 SPI 队列
    spi_queue = xQueueCreate(QUEUE_LENGTH, sizeof(uint8_t*));

    // 创建 UDP 接收任务
    xTaskCreate(udp_receive_task, "udp_receive_task", 4096, NULL, 5, NULL);

    // 创建 SPI 发送任务
    xTaskCreate(spi_send_task, "spi_send_task", 4096, NULL, 5, NULL);
}
