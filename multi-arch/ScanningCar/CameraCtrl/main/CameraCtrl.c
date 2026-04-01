#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "lwip/sockets.h"

#include "driver/ledc.h"
#include "driver/gpio.h"
// ==========================================
// 🌟 引入 ESP-IDF v5.5 原生核心组件
// ==========================================
#include "driver/jpeg_encode.h" // v5.5 鍘熺敓纭欢 JPEG 缂栫爜鍣?#10;#include "esp_cam_sensor_detect.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "imx219.h" // 浣犵殑 IMX219 椹卞姩缁勪欢
#include "injected/esp_wifi.h"
#include <linux/videodev2.h> // Linux V4L2 鏍囧噯鎺ュ彛

static const char *TAG = "P4_VIDEO_STREAM";

// ================== 网络参数配置区 ==================
#define TARGET_WIFI_SSID      "TJURM" // ⚠️ 必须是 2.4G Wi-Fi
#define TARGET_WIFI_PASS      "tjurm2020"
#define DEST_PC_IP            "192.168.28.253" // ⚠️ 运行 Python 脚本的电脑 IP
#define DEST_PC_PORT          5000
#define UDP_MTU               1400

// ================== 摄像头硬件引脚与分辨率 ==================
#define CAM_PWDN_IO             0   // IO0
#define CAM_XCLK_IO             1   // IO1
#define I2C_MASTER_SCL_IO     8
#define I2C_MASTER_SDA_IO     7
#define I2C_MASTER_NUM        0
#define I2C_MASTER_FREQ_HZ    100000
#define XCLK_FREQ_HZ          20000000

// --- Output Size 1536x1232 (Binning 2x2, Aligned 64-byte) ---
#define IMG_WIDTH             1536
#define IMG_HEIGHT            1232
#define OUT_WIDTH             640
#define OUT_HEIGHT            480

// ================== 全局变量与事件 ==================
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

int video_sock = -1;
struct sockaddr_in dest_addr;

// 图像色彩增益 (用于软件 Demosaic)
static int s_digital_gain = 128;
static int s_wb_red = 140;
static int s_wb_blue = 160;

// --- UDP 视频流自定义协议头 ---
#define PACKET_MAGIC 0x4A504547 // "JPEG"
typedef struct {
    uint32_t magic;      // 魔法数字，标识合法视频包
    uint32_t frame_id;   // 帧序号
    uint32_t total_len;  // 这一帧 JPEG 的总大小
    uint32_t offset;     // 当前切片在整帧中的偏移量
} __attribute__((packed)) video_packet_header_t;



/* ==========================================================
 * 辅助函数 1：UDP 图像切片发送
 * ========================================================== */
void send_jpeg_frame_udp(const uint8_t *jpeg_buf, uint32_t jpeg_len) {
    static uint32_t current_frame_id = 0;
    if (video_sock < 0 || jpeg_buf == NULL || jpeg_len == 0) return;

    uint32_t offset = 0;
    uint8_t packet_buf[UDP_MTU + sizeof(video_packet_header_t)];

    while (offset < jpeg_len) {
        uint32_t chunk_len = jpeg_len - offset;
        if (chunk_len > UDP_MTU) chunk_len = UDP_MTU;

        video_packet_header_t *header = (video_packet_header_t *)packet_buf;
        header->magic = PACKET_MAGIC;
        header->frame_id = current_frame_id;
        header->total_len = jpeg_len;
        header->offset = offset;

        memcpy(packet_buf + sizeof(video_packet_header_t), jpeg_buf + offset, chunk_len);

        int total_send_len = sizeof(video_packet_header_t) + chunk_len;
        sendto(video_sock, packet_buf, total_send_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        offset += chunk_len;
        esp_rom_delay_us(50); // 防止瞬间打爆底层 SPI 队列
    }
    current_frame_id++;
}


static int *s_x_lut = NULL;
static int *s_y_lut = NULL;

static void init_demosaic_luts(int width, int height) {
    s_x_lut = malloc(OUT_WIDTH * sizeof(int));
    s_y_lut = malloc(OUT_HEIGHT * sizeof(int));
    for (int y = 0; y < OUT_HEIGHT; y++) s_y_lut[y] = ((y * 192) / 100) & ~1;
    for (int x = 0; x < OUT_WIDTH; x++) s_x_lut[x] = ((x * 192) / 100) & ~1;
}

static void demosaic_bggr_to_rgb(const uint8_t *raw10, uint8_t *rgb, int width, int height) {
    int out_w = OUT_WIDTH; int out_h = OUT_HEIGHT;
    for (int y = 0; y < out_h; y++) {
        int src_y = s_y_lut[y];
        int row_offset0 = src_y * (width * 5 / 4);
        int row_offset1 = (src_y + 1) * (width * 5 / 4);
        int out_row_idx = (out_h - 1 - y) * out_w;
        for (int x = 0; x < out_w; x++) {
            int src_x = s_x_lut[x];
            int group = src_x >> 2;
            int pos_in_group = src_x % 4;
            int col_offset = group * 5 + pos_in_group;

            uint8_t b = raw10[row_offset0 + col_offset];
            uint8_t g1 = raw10[row_offset0 + col_offset + 1];
            uint8_t g2 = raw10[row_offset1 + col_offset];
            uint8_t r = raw10[row_offset1 + col_offset + 1];
            uint8_t g = (g1 + g2) >> 1;

            #define CLAMP(v) ((v) > 255 ? 255 : (v))
            uint32_t r_gain = (s_digital_gain * s_wb_red) >> 7;
            uint32_t g_gain = s_digital_gain;
            uint32_t b_gain = (s_digital_gain * s_wb_blue) >> 7;

            int out_idx = (out_row_idx + (out_w - 1 - x)) * 3;
            rgb[out_idx + 0] = CLAMP((r * r_gain) >> 7);
            rgb[out_idx + 1] = CLAMP((g * g_gain) >> 7);
            rgb[out_idx + 2] = CLAMP((b * b_gain) >> 7);
        }
    }
}

/* ==========================================================
 * 辅助函数 3：网络事件回调
 * ========================================================== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "重连 AP...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "🟢 成功获取 IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}


/* ==========================================================
 * 主函数 app_main (整合全部流程)
 * ========================================================== */
void app_main(void)
{

    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "==== ESP32-P4 + IMX219 视频推流启动 ====");

    // ==========================================
    // 1. 网络系统初始化 (基于 ESP-Hosted C6)
    // ==========================================
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta(); // 关键：拉起 DHCP

    wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TARGET_WIFI_SSID,
            .password = TARGET_WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // 阻塞等待网络联通
    ESP_LOGI(TAG, "等待 Wi-Fi (由 C6 代理) 连接...");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    // 建立 UDP 通道
    video_sock = socket(AF_INET, SOCK_DGRAM, 0);
    dest_addr.sin_addr.s_addr = inet_addr(DEST_PC_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PC_PORT);
    ESP_LOGI(TAG, "🚀 UDP 通道建立 -> %s:%d", DEST_PC_IP, DEST_PC_PORT);

    // ==========================================
    // 2. 初始化 ESP-IDF v5.5 硬件 JPEG 编码器
    // ==========================================
    ESP_LOGI(TAG, "初始化硬件 JPEG 编码器...");
    jpeg_encode_engine_cfg_t eng_cfg = {
        .timeout_ms = 3000,
    };
    jpeg_encoder_handle_t jpeg_handle = NULL;
    ESP_ERROR_CHECK(jpeg_new_encoder_engine(&eng_cfg, &jpeg_handle));

    jpeg_encode_cfg_t enc_config = {
        .width = OUT_WIDTH,
        .height = OUT_HEIGHT,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB888, // 对应 RGB24
        .sub_sample = JPEG_DOWN_SAMPLING_YUV422,
        .image_quality = 30, // 压缩质量 30，适配 SPI 带宽
    };

    // 【极度关键】在 32MB PSRAM 中预分配显存，防止 OOM 崩溃
    size_t max_jpg_size = OUT_WIDTH * OUT_HEIGHT;
    uint8_t *jpg_out_buf = heap_caps_malloc(max_jpg_size, MALLOC_CAP_SPIRAM);
    uint8_t *rgb_buf = heap_caps_malloc(OUT_WIDTH * OUT_HEIGHT * 3, MALLOC_CAP_SPIRAM);
    if (!jpg_out_buf || !rgb_buf) {
        ESP_LOGE(TAG, "🚨 严重错误：PSRAM 分配失败！请确保 menuconfig 中开启了 PSRAM 并设为 120MHz");
        return;
    }

    // ==========================================
    // 3. 初始化 IMX219 与 V4L2 摄像头节点
    // ==========================================
    // 2. 【关键移植】硬件上电与时钟初始化
    ESP_LOGI(TAG, "正在为 IMX219 准备硬件环境...");

    // A. 拉高 PWDN 引脚 (唤醒传感器)
    gpio_config_t conf = {
        .pin_bit_mask = (1ULL << CAM_PWDN_IO),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&conf);

    gpio_set_level(CAM_PWDN_IO, 0); // ✨ 先拉低，硬件复位
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CAM_PWDN_IO, 1); // ✨ 再拉高唤醒
    vTaskDelay(pdMS_TO_TICKS(10));


    // B. 启动 XCLK (24MHz)
    // 传感器必须有 XCLK 才能响应 I2C
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_1_BIT,
        .freq_hz = 24000000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .gpio_num = CAM_XCLK_IO,
        .duty = 1,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
    vTaskDelay(pdMS_TO_TICKS(100)); // 等待时钟稳定

    // 5. 打开设备

    ESP_LOGI(TAG, "初始化摄像头硬件与 V4L2 节点...");
    init_demosaic_luts(IMG_WIDTH, IMG_HEIGHT);

    vTaskDelay(pdMS_TO_TICKS(50));
    imx219_force_link(); // 强制链接底层驱动

    esp_video_init_csi_config_t csi_config = {
        .sccb_config = {
            .init_sccb = true,
            .i2c_config = {
                .port = I2C_MASTER_NUM, .scl_pin = I2C_MASTER_SCL_IO, .sda_pin = I2C_MASTER_SDA_IO,
            },
            .freq = I2C_MASTER_FREQ_HZ,
        },
        .reset_pin = -1, .pwdn_pin = -1,
    };
    esp_video_init_config_t cam_config = { .csi = &csi_config };

    // 注册设备 /dev/video0
    if (esp_video_init(&cam_config) != ESP_OK) {
        ESP_LOGE(TAG, "视频系统初始化失败");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        ESP_LOGE(TAG, "依然无法打开设备，错误码: %d", errno);
        return;
    }

    // ==========================================================
    // 第一步：先设置格式 (此时不要管 esp_cam_sensor_set_format，直接 ioctl)
    // ==========================================================
    struct v4l2_format v_fmt = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .fmt.pix.width = IMG_WIDTH,
        .fmt.pix.height = IMG_HEIGHT,
        .fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR10, // 确保这是驱动支持的像素格式
    };

    // V4L2 层的 S_FMT 会自动触发底层的传感器配置
    if (ioctl(fd, VIDIOC_S_FMT, &v_fmt) != 0) {
        ESP_LOGE(TAG, "❌ VIDIOC_S_FMT 失败！这意味着驱动不支持 1920x1080 或 SBGGR10");
        // 如果报错，请尝试将 1920/1080 改为 640/480 测试
        return;
    }
    ESP_LOGI(TAG, "✅ 格式匹配成功，MIPI 时钟已同步");

    // ==========================================================
    // 第二步：格式确定后，再申请缓冲区 (REQBUFS)
    // ==========================================================
    struct v4l2_requestbuffers req = {
        .count = 4,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP
    };
    if (ioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "❌ REQBUFS 失败");
        return;
    }

    // 映射缓冲区
    void *mapped_bufs[4];
    for (int i = 0; i < 4; i++) {
        struct v4l2_buffer b = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
            .index = i
        };
        ioctl(fd, VIDIOC_QUERYBUF, &b);
        mapped_bufs[i] = mmap(NULL, b.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, b.m.offset);

        // 初始入队
        ioctl(fd, VIDIOC_QBUF, &b);
    }

    struct v4l2_format actual_fmt = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
    if (ioctl(fd, VIDIOC_G_FMT, &actual_fmt) == 0) {
        ESP_LOGI(TAG, "🔍 硬件最终确认的分辨率: %ld x %ld",
                 actual_fmt.fmt.pix.width, actual_fmt.fmt.pix.height);
    }

    // ==========================================================
    // 第三步：启动视频流
    // ==========================================================
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "❌ STREAMON 失败");
        return;
    }

    ESP_LOGI(TAG, "🎥 硬件链路已闭环，进入采集循环...");
    // ==========================================
    // 4. 视频核心主循环 (采集 -> 去马赛克 -> 压缩 -> 推流)
    // ==========================================
    uint32_t frame_count = 0;
    uint64_t last_time = esp_timer_get_time();
    struct v4l2_buffer buf_dq = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE, .memory = V4L2_MEMORY_MMAP };

    ESP_LOGI(TAG, "开始循环");
    while (1) {
        // 出队：获取一帧原始画面
        ESP_LOGI(TAG, "等待 DQBUF...");
        if (ioctl(fd, VIDIOC_DQBUF, &buf_dq) == 0) {
            ESP_LOGI(TAG, "成功获取一帧数据，大小: %ld", buf_dq.bytesused);
            uint8_t *raw_data = (uint8_t*)mapped_bufs[buf_dq.index];
            ESP_LOGI(TAG, "开始软件去马赛克...");
            // 步骤 A: 软件拜耳去马赛克 (RAW10 -> RGB888)
            demosaic_bggr_to_rgb(raw_data, rgb_buf, IMG_WIDTH, IMG_HEIGHT);

            ESP_LOGI(TAG, "去马赛克完成，开始 JPEG 编码...");
            // 步骤 B: 硬件极速 JPEG 编码
            uint32_t out_actual_len = 0;
            esp_err_t ret = jpeg_encoder_process(jpeg_handle,
                                                 &enc_config,
                                                 rgb_buf,
                                                 OUT_WIDTH * OUT_HEIGHT * 3,
                                                 jpg_out_buf,
                                                 max_jpg_size,
                                                 &out_actual_len);

            if (ret == ESP_OK && out_actual_len > 0) {
                // 步骤 C: UDP 切片发送给电脑
                send_jpeg_frame_udp(jpg_out_buf, out_actual_len);
            } else {
                ESP_LOGE(TAG, "JPEG 压缩失败: %s", esp_err_to_name(ret));
            }

            frame_count++;

            // 入队：将处理完的缓冲区归还给底层的 DMA
            ioctl(fd, VIDIOC_QBUF, &buf_dq);
            ESP_LOGI(TAG, "Buffer 已归还 QBUF");

        } else {
            // 如果没拿到数据，让出 CPU
            ESP_LOGE(TAG, "DQBUF 失败: %s", strerror(errno));
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        // 性能与帧率统计
        uint64_t now = esp_timer_get_time();
        if ((now - last_time) >= 1000000) {
            ESP_LOGI("VIDEO_STREAM", "推流成功: %lu FPS", frame_count);
            frame_count = 0;
            last_time = now;
        }
    }
}