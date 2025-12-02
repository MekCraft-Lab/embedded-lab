#include <stdio.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

static const char *TAG = "UDP_SERVER";

void udp_server_task(void *pvParameters)
{
  char rx_buffer[128];
  char addr_str[128];
  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_UDP;

  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(12345);

  int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
  bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

  ESP_LOGI(TAG, "UDP server listening on port 12345");

  while (1) {
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                       (struct sockaddr *)&source_addr, &socklen);

    if (len > 0) {
      rx_buffer[len] = 0;
      ESP_LOGI(TAG, "Received: %s", rx_buffer);
    }
  }
}

void app_main(void)
{
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();

  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t wifi_config = {
    .ap = {
      .ssid = "C6_TEST_AP",
      .password = "12345678",
      .ssid_len = 0,
      .channel = 1,
      .max_connection = 5,
      .authmode = WIFI_AUTH_WPA_WPA2_PSK,
  },
};

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
  esp_wifi_start();

  ESP_LOGI(TAG, "AP started, SSID:C6_TEST_AP  PASSWORD:12345678");
  xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}

