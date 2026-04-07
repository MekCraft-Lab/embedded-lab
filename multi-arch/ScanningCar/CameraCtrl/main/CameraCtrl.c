#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/poll.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "lwip/sockets.h"
#include "esp_cam_sensor_xclk.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/jpeg_encode.h"
#include "esp_video_device.h"
#include "esp_video_init.h"
#include "esp_heap_caps.h"
#include "injected/esp_wifi.h"
#include <linux/videodev2.h>

static const char *TAG = "P4_VIDEO_STREAM";

// ================== 网络参数配置区 ==================
#define TARGET_WIFI_SSID      "TJURM"
#define TARGET_WIFI_PASS      "tjurm2020"
#define DEST_PC_IP            "192.168.28.125"
#define DEST_PC_PORT          5000
#define UDP_MTU               1400
#define UDP_SEND_GAP_US       0
#define V4L2_BUF_COUNT        4
#define V4L2_MEMORY_MODE      V4L2_MEMORY_USERPTR
#define MEMORY_ALIGN          64

// ================== 摄像头硬件引脚与分辨率 ==================
#define CAM_PWDN_IO           0
#define CAM_XCLK_IO           1
#define I2C_MASTER_SCL_IO     8
#define I2C_MASTER_SDA_IO     7
#define I2C_MASTER_NUM        0
#define I2C_MASTER_FREQ_HZ    100000

#define IMG_WIDTH             800
#define IMG_HEIGHT            640
#define CAMERA_FPS            50
#define DQBUF_TIMEOUT_MS      1500
#define PREFERRED_PIXFMT      V4L2_PIX_FMT_RGB565
#define PERF_REPORT_INTERVAL_US 1000000
#define ENABLE_V4L2_SET_PARM  1
#define ENABLE_V4L2_TUNE_CTRL 1

// ================== 采集统计 (capture_task 写, encode_send_task 读) ==================
typedef struct {
    uint32_t cap_ok;        // 成功出队并送入编码队列的帧数
    uint32_t poll_timeout;  // poll 超时次数
    uint32_t dqbuf_fail;    // DQBUF 失败次数
    uint32_t buf_error;     // V4L2_BUF_FLAG_ERROR 帧数
    uint32_t queue_full;    // 编码队列满导致丢弃的帧数
    uint64_t cap_total_us;  // 采集侧总累计耗时
} cap_stats_t;

static volatile cap_stats_t s_cap_stats;  // capture_task 更新

// ================== 性能统计 (encode_send_task 使用) ==================
typedef struct {
    uint32_t frames;
    uint64_t jpeg_us;
    uint64_t udp_us;
    uint64_t loop_us;
    uint64_t jpeg_bytes;
    uint64_t capture_us;        // poll + DQBUF 耗时
    uint64_t queue_wait_us;     // 帧在队列中等待耗时
    uint64_t qbuf_us;           // QBUF 归还耗时
    uint64_t frame_interval_us; // 帧间总间隔 (前一帧开始 → 当前帧开始)
    uint32_t enc_fail;          // JPEG 编码失败次数
} perf_stats_t;

// ================== 双任务流水线共享结构 ==================
typedef struct {
    struct v4l2_buffer vbuf;
    uint64_t capture_start_us;  // poll 开始时刻
    uint64_t capture_done_us;   // DQBUF 完成时刻
} frame_slot_t;

typedef struct {
    int fd;
    QueueHandle_t queue;
    void *mapped_bufs[V4L2_BUF_COUNT];
    uint32_t buf_sizes[V4L2_BUF_COUNT];
} capture_ctx_t;

typedef struct {
    int fd;
    QueueHandle_t queue;
    jpeg_encoder_handle_t jpeg_handle;
    jpeg_encode_cfg_t enc_config;
    uint8_t *jpg_out_buf;
    size_t jpg_alloc_size;
    void *mapped_bufs[V4L2_BUF_COUNT];
    uint32_t buf_sizes[V4L2_BUF_COUNT];
} encode_ctx_t;

// ================== 文件作用域静态变量 ==================
static uint32_t s_active_width = IMG_WIDTH;
static uint32_t s_active_height = IMG_HEIGHT;
static uint32_t s_active_pixfmt = PREFERRED_PIXFMT;

static capture_ctx_t s_cap_ctx;
static encode_ctx_t  s_enc_ctx;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

int video_sock = -1;
struct sockaddr_in dest_addr;

// --- UDP 视频流协议头 ---
#define PACKET_MAGIC 0x4A504547
typedef struct {
    uint32_t magic;
    uint32_t frame_id;
    uint32_t total_len;
    uint32_t offset;
} __attribute__((packed)) video_packet_header_t;

// ================== 辅助函数 ==================

// --- OV5647 I2C 寄存器读取调试 ---

static uint8_t read_sensor_reg(i2c_master_dev_handle_t dev, uint16_t reg)
{
    uint8_t buf[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    uint8_t val = 0xFF;
    if (i2c_master_transmit(dev, buf, 2, 50) == ESP_OK) {
        i2c_master_receive(dev, &val, 1, 50);
    }
    return val;
}

static uint16_t read_sensor_reg16(i2c_master_dev_handle_t dev, uint16_t reg)
{
    return ((uint16_t)read_sensor_reg(dev, reg) << 8) | read_sensor_reg(dev, reg + 1);
}

static void debug_dump_ov5647_regs(void)
{
    /* 第一步: 扫描 I2C 总线找到传感器地址 */
    i2c_master_bus_handle_t bus = NULL;
    if (i2c_master_get_bus_handle(I2C_MASTER_NUM, &bus) != ESP_OK) {
        ESP_LOGE(TAG, "Cannot get I2C bus handle");
        return;
    }

    ESP_LOGI(TAG, "===== I2C Bus Scan =====");
    uint8_t found_addr = 0xFF;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_master_probe(bus, addr, 50) == ESP_OK) {
            ESP_LOGI(TAG, "  Found device at 0x%02X", addr);
            if (found_addr == 0xFF) found_addr = addr;
        }
    }
    if (found_addr == 0xFF) {
        ESP_LOGE(TAG, "No I2C device found on bus!");
        return;
    }

    /* 第二步: 用找到的地址读寄存器 */
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = found_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev = NULL;
    if (i2c_master_bus_add_device(bus, &cfg, &dev) != ESP_OK) {
        ESP_LOGE(TAG, "Cannot add I2C device at 0x%02X", found_addr);
        return;
    }

    ESP_LOGI(TAG, "===== OV5647 Register Dump (addr=0x%02X) =====", found_addr);

    // Chip ID
    ESP_LOGI(TAG, "[ID] PID=0x%02X%02X (expect 0x5647)",
             read_sensor_reg(dev, 0x300A), read_sensor_reg(dev, 0x300B));

    // PLL - 完整读取, 包含之前缺失的关键寄存器
    uint8_t r3034 = read_sensor_reg(dev, 0x3034);
    uint8_t r3035 = read_sensor_reg(dev, 0x3035);
    uint8_t r3036 = read_sensor_reg(dev, 0x3036);
    uint8_t r3037 = read_sensor_reg(dev, 0x3037);
    uint8_t r303c = read_sensor_reg(dev, 0x303c);
    uint8_t r3106 = read_sensor_reg(dev, 0x3106);
    uint8_t r3108 = read_sensor_reg(dev, 0x3108);
    ESP_LOGI(TAG, "[PLL] 0x3034=0x%02X(mode) 0x3035=0x%02X(sys_div) 0x3036=0x%02X(mult) 0x3037=0x%02X(pre/root)",
             r3034, r3035, r3036, r3037);
    ESP_LOGI(TAG, "[PLL2] 0x303c=0x%02X(pll_sys) 0x3106=0x%02X(clk_div) 0x3108=0x%02X(pclk_div)",
             r303c, r3106, r3108);
    ESP_LOGI(TAG, "[PLL3] 0x3038=0x%02X 0x3039=0x%02X",
             read_sensor_reg(dev, 0x3038), read_sensor_reg(dev, 0x3039));

    // 用驱动中 ov5647_get_sysclk() 的算法计算理论 PCLK
    static const int pre_div02x_map[] = {2, 2, 4, 6, 8, 3, 12, 5, 16, 2, 2, 2, 2, 2, 2, 2};
    static const int sdiv0_map[] = {16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    static const int pll_rdiv_map[] = {1, 2};
    static const int bit_div2x_map[] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 5, 2, 2, 2, 2, 2};
    static const int sclk_div_map[] = {1, 2, 4, 1};

    int xvclk = 2400; // 24MHz / 10000
    int pre_div02x = pre_div02x_map[r3037 & 0x0f];
    int pll_rdiv = pll_rdiv_map[(r3037 >> 4) & 0x01];
    int div_cnt7b = r3036;
    int VCO = xvclk * 2 / pre_div02x * div_cnt7b;
    int sdiv0 = sdiv0_map[r3035 >> 4];
    int bit_div2x = bit_div2x_map[r3034 & 0x0f];
    int sclk_div = sclk_div_map[(r3106 >> 2) & 0x03];
    int sysclk = VCO * 2 / sdiv0 / pll_rdiv / bit_div2x / sclk_div;

    // Frame timing
    uint16_t vts = read_sensor_reg16(dev, 0x380C);
    uint16_t hts = read_sensor_reg16(dev, 0x380E);
    uint32_t frame_pixels = (uint32_t)vts * hts;
    int calc_fps = frame_pixels > 0 ? (sysclk * 10000 / frame_pixels) : 0;
    ESP_LOGI(TAG, "[TIMING] VTS=%u HTS=%u frame_len=%u", vts, hts, frame_pixels);
    ESP_LOGI(TAG, "[CALC] VCO=%dMHz sysclk=%dMHz(×10k) => calc_fps=%d",
             VCO / 100, sysclk / 100, calc_fps);
    ESP_LOGI(TAG, "[CALC_DETAIL] pre_div02x=%d pll_rdiv=%d mult=%d sdiv0=%d bit_div2x=%d sclk_div=%d",
             pre_div02x, pll_rdiv, div_cnt7b, sdiv0, bit_div2x, sclk_div);

    // Window
    ESP_LOGI(TAG, "[WINDOW] X[%u..%u] Y[%u..%u]",
             read_sensor_reg16(dev, 0x3800), read_sensor_reg16(dev, 0x3802),
             read_sensor_reg16(dev, 0x3804), read_sensor_reg16(dev, 0x3806));

    // Output size
    ESP_LOGI(TAG, "[OUTPUT_SIZE] %ux%u",
             read_sensor_reg16(dev, 0x3808), read_sensor_reg16(dev, 0x380A));

    // Sub-sampling / scaling
    ESP_LOGI(TAG, "[SUBSAMPLE] 0x3814=0x%02X 0x3815=0x%02X 0x3820=0x%02X 0x3821=0x%02X",
             read_sensor_reg(dev, 0x3814), read_sensor_reg(dev, 0x3815),
             read_sensor_reg(dev, 0x3820), read_sensor_reg(dev, 0x3821));

    // PCLK dividers
    ESP_LOGI(TAG, "[PCLK] 0x3824=0x%02X(odd) 0x3825=0x%02X(even)",
             read_sensor_reg(dev, 0x3824), read_sensor_reg(dev, 0x3825));

    // Output format
    ESP_LOGI(TAG, "[FMT] 0x4300=0x%02X 0x501F=0x%02X",
             read_sensor_reg(dev, 0x4300), read_sensor_reg(dev, 0x501F));

    // MIPI
    ESP_LOGI(TAG, "[MIPI] 0x4800=0x%02X 0x4837=0x%02X",
             read_sensor_reg(dev, 0x4800), read_sensor_reg(dev, 0x4837));

    // Clock enables
    ESP_LOGI(TAG, "[CLK_EN] 0x3000=0x%02X 0x3001=0x%02X 0x3002=0x%02X 0x3003=0x%02X",
             read_sensor_reg(dev, 0x3000), read_sensor_reg(dev, 0x3001),
             read_sensor_reg(dev, 0x3002), read_sensor_reg(dev, 0x3003));

    // Mode / streaming
    uint8_t r0100 = read_sensor_reg(dev, 0x0100);
    ESP_LOGI(TAG, "[MODE] 0x0100=0x%02X (streaming=%d)", r0100, r0100 & 1);

    // 800x1280 模式有但 800x640 没有的关键寄存器
    ESP_LOGI(TAG, "[EXTRA] 0x3016=0x%02X 0x3017=0x%02X 0x3018=0x%02X 0x301c=0x%02X 0x301d=0x%02X",
             read_sensor_reg(dev, 0x3016), read_sensor_reg(dev, 0x3017),
             read_sensor_reg(dev, 0x3018), read_sensor_reg(dev, 0x301c),
             read_sensor_reg(dev, 0x301d));

    i2c_master_bus_rm_device(dev);
    ESP_LOGI(TAG, "===== End OV5647 Dump =====");
}

static void send_jpeg_frame_udp(const uint8_t *jpeg_buf, uint32_t jpeg_len) {
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
        sendto(video_sock, packet_buf, sizeof(video_packet_header_t) + chunk_len,
               0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        offset += chunk_len;
        if (UDP_SEND_GAP_US > 0) {
            esp_rom_delay_us(UDP_SEND_GAP_US);
        }
    }
    current_frame_id++;
}

static esp_err_t wait_for_frame_ready(int fd, int timeout_ms)
{
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
        ESP_LOGE(TAG, "poll failed: %s", strerror(errno));
        return ESP_FAIL;
    }
    if (ret == 0) {
        ESP_LOGW(TAG, "wait frame timeout (%d ms)", timeout_ms);
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

static esp_err_t configure_capture_format(int fd, uint32_t prefer_w, uint32_t prefer_h, uint32_t prefer_pixfmt)
{
    const uint32_t try_pixfmts[] = {
        prefer_pixfmt,
        V4L2_PIX_FMT_SBGGR10,
        V4L2_PIX_FMT_SBGGR8,
        V4L2_PIX_FMT_RGB565,
        V4L2_PIX_FMT_YUYV,
    };
    const uint32_t try_sizes[][2] = {
        {prefer_w, prefer_h},
        {800, 640},
        {640, 480},
    };

    for (size_t pf = 0; pf < sizeof(try_pixfmts) / sizeof(try_pixfmts[0]); pf++) {
        for (size_t sz = 0; sz < sizeof(try_sizes) / sizeof(try_sizes[0]); sz++) {
            struct v4l2_format fmt = {
                .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
                .fmt.pix.width = try_sizes[sz][0],
                .fmt.pix.height = try_sizes[sz][1],
                .fmt.pix.pixelformat = try_pixfmts[pf],
            };
            if (ioctl(fd, VIDIOC_S_FMT, &fmt) == 0) {
                s_active_width = fmt.fmt.pix.width;
                s_active_height = fmt.fmt.pix.height;
                s_active_pixfmt = fmt.fmt.pix.pixelformat;
                ESP_LOGI(TAG, "S_FMT: pixfmt=0x%08lx, %lux%lu",
                         (unsigned long)fmt.fmt.pix.pixelformat,
                         (unsigned long)s_active_width, (unsigned long)s_active_height);
                return ESP_OK;
            }
        }
    }

    ESP_LOGW(TAG, "Direct S_FMT failed, enumerating...");
    for (uint32_t fi = 0; ; fi++) {
        struct v4l2_fmtdesc fmtdesc = { .index = fi, .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != 0) break;

        struct v4l2_frmsizeenum fsize = {
            .index = 0,
            .pixel_format = fmtdesc.pixelformat,
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        };
        if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize) != 0) continue;

        for (uint32_t si = 0; ; si++) {
            fsize.index = si;
            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize) != 0) break;
            if (fsize.type != V4L2_FRMSIZE_TYPE_DISCRETE) continue;

            struct v4l2_format try_fmt = {
                .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
                .fmt.pix.width = fsize.discrete.width,
                .fmt.pix.height = fsize.discrete.height,
                .fmt.pix.pixelformat = fmtdesc.pixelformat,
            };
            if (ioctl(fd, VIDIOC_S_FMT, &try_fmt) == 0) {
                s_active_width = try_fmt.fmt.pix.width;
                s_active_height = try_fmt.fmt.pix.height;
                s_active_pixfmt = try_fmt.fmt.pix.pixelformat;
                ESP_LOGI(TAG, "Enum S_FMT: pixfmt=0x%08lx, %lux%lu",
                         (unsigned long)try_fmt.fmt.pix.pixelformat,
                         (unsigned long)s_active_width, (unsigned long)s_active_height);
                return ESP_OK;
            }
        }
    }
    return ESP_FAIL;
}

static esp_err_t set_stream_fps(int fd, uint32_t prefer_fps)
{
    // 先通过 ENUM_FRAMEINTERVALS 查询驱动支持的精确帧间隔
    struct v4l2_frmivalenum frmival = {
        .index = 0,
        .pixel_format = s_active_pixfmt,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .width = s_active_width,
        .height = s_active_height,
    };

    bool set_ok = false;

    if (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0) {
        ESP_LOGI(TAG, "ENUM_FRAMEINTERVALS[0]: %u/%u (requested %u fps)",
                 frmival.discrete.numerator, frmival.discrete.denominator, prefer_fps);

        struct v4l2_streamparm sparm = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .parm.capture.capability = V4L2_CAP_TIMEPERFRAME,
            .parm.capture.timeperframe.numerator = frmival.discrete.numerator,
            .parm.capture.timeperframe.denominator = frmival.discrete.denominator,
        };
        set_ok = (ioctl(fd, VIDIOC_S_PARM, &sparm) == 0);
        if (!set_ok) {
            ESP_LOGW(TAG, "S_PARM with enum value failed: %s", strerror(errno));
        }
    } else {
        ESP_LOGW(TAG, "ENUM_FRAMEINTERVALS failed: %s, trying direct S_PARM", strerror(errno));
    }

    // fallback: 直接用请求的 fps 设置
    if (!set_ok) {
        struct v4l2_streamparm sparm = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .parm.capture.capability = V4L2_CAP_TIMEPERFRAME,
            .parm.capture.timeperframe.numerator = 1,
            .parm.capture.timeperframe.denominator = prefer_fps,
        };
        set_ok = (ioctl(fd, VIDIOC_S_PARM, &sparm) == 0);
        if (!set_ok) {
            ESP_LOGW(TAG, "S_PARM denominator=%u failed: %s", prefer_fps, strerror(errno));
        }
    }

    // 读取实际生效的参数
    struct v4l2_streamparm gparm = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
    if (ioctl(fd, VIDIOC_G_PARM, &gparm) == 0) {
        ESP_LOGI(TAG, "Active timeperframe: %lu/%lu (set_ok=%d)",
                 (unsigned long)gparm.parm.capture.timeperframe.numerator,
                 (unsigned long)gparm.parm.capture.timeperframe.denominator,
                 set_ok);
    } else {
        ESP_LOGW(TAG, "G_PARM failed: %s", strerror(errno));
    }

    return set_ok ? ESP_OK : ESP_FAIL;
}

static void try_set_ctrl(int fd, uint32_t id, int32_t value, const char *name)
{
    struct v4l2_control ctrl = { .id = id, .value = value };
    if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) == 0) {
        ESP_LOGI(TAG, "set ctrl %s=%ld", name, (long)value);
    } else {
        ESP_LOGW(TAG, "set ctrl %s failed: %s", name, strerror(errno));
    }
}

static void tune_v4l2_controls_for_fps(int fd)
{
#if ENABLE_V4L2_TUNE_CTRL
    try_set_ctrl(fd, V4L2_CID_AUTOGAIN, 0, "AUTOGAIN");
    try_set_ctrl(fd, V4L2_CID_GAIN, 64, "GAIN");
    try_set_ctrl(fd, V4L2_CID_EXPOSURE_AUTO, V4L2_EXPOSURE_MANUAL, "EXPOSURE_AUTO");
    try_set_ctrl(fd, V4L2_CID_EXPOSURE, 64, "EXPOSURE");
    try_set_ctrl(fd, V4L2_CID_EXPOSURE_ABSOLUTE, 50, "EXPOSURE_ABSOLUTE");
#else
    (void)fd;
#endif
}

// ================== 双任务函数 (文件作用域) ==================

static void capture_task(void *arg)
{
    capture_ctx_t *ctx = (capture_ctx_t *)arg;
    ESP_LOGI(TAG, "[CAP] Capture task started on core %d", xPortGetCoreID());

    while (1) {
        frame_slot_t slot;
        memset(&slot, 0, sizeof(slot));
        slot.vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        slot.vbuf.memory = V4L2_MEMORY_MODE;

        // 阻塞式 DQBUF (与 capture_stream 一致, 不用 poll)
        if (ioctl(ctx->fd, VIDIOC_DQBUF, &slot.vbuf) != 0) {
            ESP_LOGE(TAG, "[CAP] DQBUF failed: %s", strerror(errno));
            s_cap_stats.dqbuf_fail++;
            continue;
        }
        s_cap_stats.cap_ok++;

        if (slot.vbuf.flags & V4L2_BUF_FLAG_ERROR) {
            s_cap_stats.buf_error++;
            slot.vbuf.m.userptr = (unsigned long)ctx->mapped_bufs[slot.vbuf.index];
            slot.vbuf.length = ctx->buf_sizes[slot.vbuf.index];
            ioctl(ctx->fd, VIDIOC_QBUF, &slot.vbuf);
            continue;
        }

        // 推入编码队列, 若队列满则立即归还 buffer
        if (xQueueSend(ctx->queue, &slot, 0) != pdTRUE) {
            s_cap_stats.queue_full++;
            slot.vbuf.m.userptr = (unsigned long)ctx->mapped_bufs[slot.vbuf.index];
            slot.vbuf.length = ctx->buf_sizes[slot.vbuf.index];
            ioctl(ctx->fd, VIDIOC_QBUF, &slot.vbuf);
        }

        // 每秒报告原始帧率
        {
            static uint32_t raw_count = 0;
            static uint64_t raw_t0 = 0;
            uint64_t now = esp_timer_get_time();
            if (raw_t0 == 0) raw_t0 = now;
            raw_count++;
            if (now - raw_t0 >= 1000000) {
                ESP_LOGI("RAW_FPS", "raw_sensor_fps=%lu  (%lu frames in %luus)",
                         raw_count, raw_count, (uint32_t)(now - raw_t0));
                raw_count = 0;
                raw_t0 = now;
            }
        }
    }
}

static void encode_send_task(void *arg)
{
    encode_ctx_t *ctx = (encode_ctx_t *)arg;
    ESP_LOGI(TAG, "[ENC] Encode+Send task started on core %d", xPortGetCoreID());

    uint32_t frame_count = 0;
    uint64_t last_time = esp_timer_get_time();
    uint64_t prev_frame_start = 0;
    perf_stats_t perf = {0};

    while (1) {
        frame_slot_t slot;
        if (xQueueReceive(ctx->queue, &slot, pdMS_TO_TICKS(2000)) != pdTRUE) {
            ESP_LOGW(TAG, "[ENC] Queue receive timeout");
            continue;
        }

        uint64_t loop_t0 = esp_timer_get_time();
        uint64_t now = loop_t0;

        // 帧间总间隔
        if (prev_frame_start > 0) {
            perf.frame_interval_us += (now - prev_frame_start);
        }
        prev_frame_start = now;

        // 采集耗时 (poll + DQBUF)
        perf.capture_us += (slot.capture_done_us - slot.capture_start_us);
        // 队列等待耗时
        perf.queue_wait_us += (now - slot.capture_done_us);

        uint8_t *raw_data = (uint8_t *)ctx->mapped_bufs[slot.vbuf.index];

        uint32_t jpeg_in_size;
        if (ctx->enc_config.src_type == JPEG_ENCODE_IN_FORMAT_GRAY) {
            jpeg_in_size = s_active_width * s_active_height;
        } else {
            jpeg_in_size = s_active_width * s_active_height * 2;
        }

        // JPEG 硬件编码
        uint64_t t_jpeg0 = esp_timer_get_time();
        uint32_t out_actual_len = 0;
        esp_err_t ret = jpeg_encoder_process(ctx->jpeg_handle,
                                             &ctx->enc_config,
                                             raw_data, jpeg_in_size,
                                             ctx->jpg_out_buf, (uint32_t)ctx->jpg_alloc_size,
                                             &out_actual_len);
        perf.jpeg_us += (esp_timer_get_time() - t_jpeg0);

        if (ret == ESP_OK && out_actual_len > 0) {
            // UDP 切片发送
            uint64_t t_udp0 = esp_timer_get_time();
            send_jpeg_frame_udp(ctx->jpg_out_buf, out_actual_len);
            perf.udp_us += (esp_timer_get_time() - t_udp0);
            perf.jpeg_bytes += out_actual_len;
        } else {
            ESP_LOGE(TAG, "[ENC] JPEG encode failed: %s", esp_err_to_name(ret));
            perf.enc_fail++;
        }

        // 归还 buffer 给 DMA
        uint64_t t_qbuf0 = esp_timer_get_time();
        // USERPTR 模式需要在 QBUF 前恢复 userptr
        slot.vbuf.m.userptr = (unsigned long)ctx->mapped_bufs[slot.vbuf.index];
        slot.vbuf.length = ctx->buf_sizes[slot.vbuf.index];
        ioctl(ctx->fd, VIDIOC_QBUF, &slot.vbuf);
        perf.qbuf_us += (esp_timer_get_time() - t_qbuf0);

        frame_count++;
        perf.frames++;
        perf.loop_us += (esp_timer_get_time() - loop_t0);

        // 性能统计 (每秒输出)
        uint64_t now_perf = esp_timer_get_time();
        if ((now_perf - last_time) >= PERF_REPORT_INTERVAL_US) {
            uint32_t frames = perf.frames ? perf.frames : 1;
            uint64_t other_us = perf.loop_us - perf.jpeg_us - perf.udp_us - perf.qbuf_us;
            uint32_t cap_ok = s_cap_stats.cap_ok ? s_cap_stats.cap_ok : 1;
            ESP_LOGI("VIDEO_STREAM",
                     "enc_fps=%lu cap_avg=%luus qwait=%luus jpeg=%luus udp=%luus qbuf=%luus other=%luus size=%luB",
                     frame_count,
                     (uint32_t)(s_cap_stats.cap_total_us / cap_ok),
                     (uint32_t)(perf.queue_wait_us / frames),
                     (uint32_t)(perf.jpeg_us / frames),
                     (uint32_t)(perf.udp_us / frames),
                     (uint32_t)(perf.qbuf_us / frames),
                     (uint32_t)(other_us / frames),
                     (uint32_t)(perf.jpeg_bytes / frames));
            ESP_LOGI("VIDEO_STREAM",
                     "[CAP] ok=%lu err=%lu dqbuf_fail=%lu poll_tmo=%lu qfull=%lu enc_fail=%lu",
                     s_cap_stats.cap_ok, s_cap_stats.buf_error,
                     s_cap_stats.dqbuf_fail, s_cap_stats.poll_timeout,
                     s_cap_stats.queue_full, perf.enc_fail);
            frame_count = 0;
            memset(&perf, 0, sizeof(perf));
            memset((cap_stats_t *)&s_cap_stats, 0, sizeof(s_cap_stats));
            prev_frame_start = 0;
            last_time = now_perf;
        }
    }
}

// ================== 网络事件回调 ==================

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Reconnecting AP...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

// ================== 主函数 ==================

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "==== ESP32-P4 + OV5647 video stream ====");

    // 1. 网络初始化
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = { .ssid = TARGET_WIFI_SSID, .password = TARGET_WIFI_PASS },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Waiting for WiFi...");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    video_sock = socket(AF_INET, SOCK_DGRAM, 0);
    dest_addr.sin_addr.s_addr = inet_addr(DEST_PC_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PC_PORT);
    ESP_LOGI(TAG, "UDP -> %s:%d", DEST_PC_IP, DEST_PC_PORT);

    // 2. JPEG 编码器初始化
    jpeg_encode_engine_cfg_t eng_cfg = { .timeout_ms = 3000 };
    jpeg_encoder_handle_t jpeg_handle = NULL;
    ESP_ERROR_CHECK(jpeg_new_encoder_engine(&eng_cfg, &jpeg_handle));

    jpeg_encode_cfg_t enc_config = {
        .width = IMG_WIDTH,
        .height = IMG_HEIGHT,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB888,
        .sub_sample = JPEG_DOWN_SAMPLING_YUV422,
        .image_quality = 20,
    };

    size_t max_jpg_size = IMG_WIDTH * IMG_HEIGHT * 2;
    jpeg_encode_memory_alloc_cfg_t out_mem_cfg = {
        .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
    };
    size_t jpg_alloc_size = 0;
    uint8_t *jpg_out_buf = jpeg_alloc_encoder_mem(max_jpg_size, &out_mem_cfg, &jpg_alloc_size);
    if (!jpg_out_buf) {
        ESP_LOGE(TAG, "JPEG output buffer alloc failed");
        return;
    }

    // 3. 摄像头硬件初始化
    gpio_config_t conf = { .pin_bit_mask = (1ULL << CAM_PWDN_IO), .mode = GPIO_MODE_OUTPUT };
    gpio_config(&conf);
    gpio_set_level(CAM_PWDN_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CAM_PWDN_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // XCLK: 使用 ESP clock router 从 480MHz SPLL 精确分频到 24MHz
    esp_cam_sensor_xclk_handle_t xclk_handle = NULL;
    ESP_ERROR_CHECK(esp_cam_sensor_xclk_allocate(ESP_CAM_SENSOR_XCLK_ESP_CLOCK_ROUTER, &xclk_handle));
    esp_cam_sensor_xclk_config_t xclk_config = {
        .esp_clock_router_cfg = {
            .xclk_pin = CAM_XCLK_IO,
            .xclk_freq_hz = 24000000,
        }
    };
    ESP_ERROR_CHECK(esp_cam_sensor_xclk_start(xclk_handle, &xclk_config));
    ESP_LOGI(TAG, "XCLK 24MHz via ESP clock router on GPIO %d", CAM_XCLK_IO);
    vTaskDelay(pdMS_TO_TICKS(100));

    vTaskDelay(pdMS_TO_TICKS(50));

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

    if (esp_video_init(&cam_config) != ESP_OK) {
        ESP_LOGE(TAG, "Video system init failed");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Cannot open video device: %d", errno);
        return;
    }

    struct v4l2_capability capability = {0};
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) != 0) {
        ESP_LOGE(TAG, "VIDIOC_QUERYCAP failed: %s", strerror(errno));
        return;
    }
    ESP_LOGI(TAG, "Video Driver: %s, Card: %s", capability.driver, capability.card);

    if (configure_capture_format(fd, IMG_WIDTH, IMG_HEIGHT, PREFERRED_PIXFMT) != ESP_OK) {
        ESP_LOGE(TAG, "VIDIOC_S_FMT failed");
        return;
    }
    tune_v4l2_controls_for_fps(fd);

    // 按采集像素格式设置 JPEG 编码输入
    enc_config.width = s_active_width;
    enc_config.height = s_active_height;
    if (s_active_pixfmt == V4L2_PIX_FMT_SBGGR8) {
        enc_config.src_type = JPEG_ENCODE_IN_FORMAT_GRAY;
        enc_config.sub_sample = JPEG_DOWN_SAMPLING_GRAY;
    } else if (s_active_pixfmt == V4L2_PIX_FMT_RGB565) {
        enc_config.src_type = JPEG_ENCODE_IN_FORMAT_RGB565;
        enc_config.sub_sample = JPEG_DOWN_SAMPLING_YUV422;
    } else if (s_active_pixfmt == V4L2_PIX_FMT_YUYV) {
        enc_config.src_type = JPEG_ENCODE_IN_FORMAT_YUV422;
        enc_config.sub_sample = JPEG_DOWN_SAMPLING_YUV422;
    } else {
        ESP_LOGE(TAG, "Unsupported pixfmt 0x%08lx", (unsigned long)s_active_pixfmt);
        return;
    }

    if (ENABLE_V4L2_SET_PARM) {
        if (set_stream_fps(fd, CAMERA_FPS) != ESP_OK) {
            ESP_LOGW(TAG, "Using driver default FPS");
        }
    }
    ESP_LOGI(TAG, "Format OK, MIPI clock synced");

    // 调试: 在 STREAMON 之后读取 OV5647 关键寄存器 (查看实际流状态)
    // 注意: 已移到 STREAMON 之后, 这样可以看到 streaming=1 时的寄存器值

    // 申请 V4L2 缓冲区 (USERPTR 模式, PSRAM 对齐分配)
    struct v4l2_requestbuffers req = {
        .count = V4L2_BUF_COUNT,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MODE
    };
    if (ioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "REQBUFS failed");
        return;
    }

    void *mapped_bufs[V4L2_BUF_COUNT];
    uint32_t buf_sizes[V4L2_BUF_COUNT];
    for (int i = 0; i < V4L2_BUF_COUNT; i++) {
        struct v4l2_buffer b = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MODE,
            .index = i
        };
        if (ioctl(fd, VIDIOC_QUERYBUF, &b) != 0) {
            ESP_LOGE(TAG, "QUERYBUF[%d] failed: %s", i, strerror(errno));
            return;
        }

        mapped_bufs[i] = heap_caps_aligned_alloc(MEMORY_ALIGN, b.length,
                                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_CACHE_ALIGNED);
        if (!mapped_bufs[i]) {
            ESP_LOGE(TAG, "PSRAM alloc[%d] failed (size=%u)", i, b.length);
            return;
        }
        buf_sizes[i] = b.length;
        ESP_LOGI(TAG, "Buffer[%d]: %u bytes at %p (PSRAM USERPTR)", i, b.length, mapped_bufs[i]);

        b.m.userptr = (unsigned long)mapped_bufs[i];
        b.length = buf_sizes[i];
        if (ioctl(fd, VIDIOC_QBUF, &b) != 0) {
            ESP_LOGE(TAG, "QBUF[%d] failed: %s", i, strerror(errno));
            return;
        }
    }

    struct v4l2_format actual_fmt = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };
    if (ioctl(fd, VIDIOC_G_FMT, &actual_fmt) == 0) {
        ESP_LOGI(TAG, "Final resolution: %ld x %ld",
                 actual_fmt.fmt.pix.width, actual_fmt.fmt.pix.height);
    }

    // 启动视频流
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "STREAMON failed");
        return;
    }

    // 调试: 在 STREAMON 之后读取寄存器, 此时 streaming=1, PLL 应已锁定
    debug_dump_ov5647_regs();

    // 4. 启动双任务流水线
    ESP_LOGI(TAG, "Starting dual-task pipeline...");

    QueueHandle_t frame_queue = xQueueCreate(V4L2_BUF_COUNT, sizeof(frame_slot_t));
    if (!frame_queue) {
        ESP_LOGE(TAG, "Failed to create frame queue");
        return;
    }

    s_cap_ctx.fd = fd;
    s_cap_ctx.queue = frame_queue;
    memcpy(s_cap_ctx.mapped_bufs, mapped_bufs, sizeof(mapped_bufs));
    memcpy(s_cap_ctx.buf_sizes, buf_sizes, sizeof(buf_sizes));

    s_enc_ctx.fd = fd;
    s_enc_ctx.queue = frame_queue;
    s_enc_ctx.jpeg_handle = jpeg_handle;
    s_enc_ctx.enc_config = enc_config;
    s_enc_ctx.jpg_out_buf = jpg_out_buf;
    s_enc_ctx.jpg_alloc_size = jpg_alloc_size;
    memcpy(s_enc_ctx.mapped_bufs, mapped_bufs, sizeof(mapped_bufs));
    memcpy(s_enc_ctx.buf_sizes, buf_sizes, sizeof(buf_sizes));

    // 核心0: 采集, 核心1: 编码+发送 → 真正并行
    xTaskCreatePinnedToCore(capture_task, "cam_cap", 4096, &s_cap_ctx, 5, NULL, 0);
    xTaskCreatePinnedToCore(encode_send_task, "cam_enc", 8192, &s_enc_ctx, 4, NULL, 1);
}
