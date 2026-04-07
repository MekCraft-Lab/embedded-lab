# CameraCtrl 调试进度交接文档

> 日期: 2026-04-05
> 项目: ESP32-P4 + OV5647 视频流系统
> 目标: 诊断并修复 0.67fps 低帧率问题

---

## 1. 系统架构

```
OV5647 Sensor ──MIPI CSI (2-lane)──> ESP32-P4 CSI/ISP ──V4L2──> JPEG HW Encode ──UDP WiFi──> PC
```

- **MCU**: ESP32-P4 双核 360MHz, 32MB PSRAM
- **Camera**: OV5647 (SCCB addr=0x36), MIPI CSI 2-lane
- **WiFi**: ESP32-C6 协处理器 via ESP-Hosted SPI
- **分辨率**: 800x640 RAW8 (SBGGR8 Bayer)
- **预期帧率**: 50fps (menuconfig 配置)
- **实际帧率**: ~0.67fps (问题所在)

## 2. 双任务流水线架构

- **capture_task** (Core 0, prio 5): poll → DQBUF → 推入队列
- **encode_send_task** (Core 1, prio 4): 从队列取帧 → JPEG编码 → UDP发送

关键文件: `main/CameraCtrl.c`

## 3. 已确认的测量数据

### 性能分解 (每帧)
| 阶段 | 耗时 | 占比 |
|------|------|------|
| capture (poll+DQBUF) | ~1,500,000us | 99.97% |
| JPEG 编码 | ~5,600us | 0.4% |
| UDP 发送 | ~1,100us | 0.07% |
| QBUF 归还 | ~30us | ~0% |

**结论: 瓶颈在 capture 阶段, 即传感器出帧间隔约 1.5 秒**

### 采集侧统计
- cap_ok=1, poll_timeout=0, dqbuf_fail=0, buf_error=0, queue_full=0
- **无任何错误, 传感器确实在 ~1.5 秒产出一帧**

## 4. OV5647 寄存器完整 Dump (STREAMON 之后)

```
[ID]     PID=0x5647 (正确)
[PLL]    0x3034=0x18(mode) 0x3035=0x41 0x3036=0x80(mult=128) 0x3037=0x03
[PLL2]   0x303c=0x11 0x3106=0xF5 0x3108=0x00
[TIMING] VTS=1896 HTS=984
[WINDOW] X[500..0] Y[2623..1953]  ← X start > end, 存疑
[OUTPUT] 800x640
[SUBSAMPLE] 0x3814=0x31 0x3815=0x31 0x3820=0x41 0x3821=0x03
[MIPI]   0x4800=0x14(LINE_SYNC_EN) 0x4837=0x28
[CLK_EN] 0x3000=0x00 0x3001=0x00 0x3002=0x00 0x3003=0x00
[MODE]   0x0100=0x01 (streaming=1)
[EXTRA]  0x3016=0x08 0x3017=0xE0 0x3018=0x44 0x301c=0xF8 0x301d=0xF0
```

### PLL 理论计算 (用驱动中 ov5647_get_sysclk 算法)
```
XCLK=24MHz, mult=128, pre_div02x=6, pll_rdiv=1
VCO = 24M × 2 / 6 × 128 = 1024 MHz
sdiv0=4, bit_div2x=4, sclk_div=2
sysclk = 1024M × 2 / 4 / 1 / 4 / 2 = 64 MHz
理论 FPS = 64M / (1896 × 984) ≈ 34 fps
```

**所有 PLL 寄存器值与驱动期望值完全匹配, 但实际帧率是 0.67fps, 慢了约 51 倍**

## 5. 关键发现

### 发现 A: CSI 驱动的帧跳过机制 (最可能的根因)

在 CSI 驱动 `esp_video_csi_device.c` 中:
```c
// line 255-273: 每帧到来时的处理
if (!param->skip_count) {
    CAPTURE_VIDEO_DONE_BUF(video, trans->buffer, trans->received_size);  // 交付给用户
} else {
    CAPTURE_VIDEO_SKIP_BUF(video, trans->buffer);  // 丢弃
}
if (param->skip_frames) {
    param->skip_count = (param->skip_count + 1) % param->skip_frames;
}
```

**skip_frames 的含义**: 每 `skip_frames` 帧才交付 1 帧给用户, 其余丢弃。
- `skip_frames=0` = 不跳帧, 每帧都交付
- `skip_frames=50` = 每 50 帧交付 1 帧, 即 50fps → 1fps

**验证**: 如果 sensor_format.fps=50 且 skip_frames=50:
- 有效 FPS = 50 / 50 = 1fps (接近观测的 0.67fps, 差异因实际 PLL 输出 ~34fps)
- 34fps / 51 ≈ 0.67fps **完全吻合!**

### 发现 B: VIDIOC_S_PARM 失败原因

CSI 驱动 `csi_video_set_parm` 只接受 sensor_fps 的整数因数:
```c
// sensor_format.fps = 50, 请求 30fps
if ((30 > 50) || (50 % 30 != 0)) {  // 50%30=20 ≠ 0
    ESP_LOGE("denominator=30 is invalid");  // 被拒绝
}
```

50fps 的有效值为: **1, 2, 5, 10, 25, 50**。30 不在其中。

### 发现 C: SCCB 地址确认

- 驱动中 `OV5647_SCCB_ADDR = 0x36` (在 `ov5647.h:14`)
- I2C 总线扫描确认设备在 `0x36`
- 地址匹配, 不是问题

### 发现 D: V4L2 Controls 全部失败

AUTOGAIN, GAIN, EXPOSURE_AUTO, EXPOSURE 等 V4L2 控制全部返回 Invalid argument。
这是 OV5647 驱动不通过 V4L2 暴露这些控制导致的, 与帧率无关。

## 6. 待验证的根因假设

### 假设 1: skip_frames 被错误设为 50 (最可能)

VIDIOC_S_PARM 失败后, `skip_frames` 可能有非零默认值, 或者在某处被设为 sensor fps (50)。

**验证方法**:
1. 读取 `esp_video_ioctl.c` 中 VIDIOC_S_PARM 的完整逻辑
2. 检查 `skip_frames` 的初始化/默认值
3. 在 csi_video_set_parm 中加日志打印 skip_frames

**修复方向**:
- 成功调用 VIDIOC_S_PARM, 传 denominator=50 → skip_frames=1 (不跳帧)
- 或直接修改 CSI 驱动确保 skip_frames 默认为 0

### 假设 2: XCLK 频率不正确

代码使用 LEDC 在 GPIO 1 上生成 24MHz XCLK:
```c
lelc_timer_config_t ledc_timer = { .freq_hz = 24000000, .clk_cfg = LEDC_AUTO_CLK };
```

但 sdkconfig 配置为使用 ESP clock router:
```
CONFIG_CAMERA_XCLK_USE_ESP_CLOCK_ROUTER=y
```

两者可能冲突。如果实际 XCLK 不是 24MHz, PLL 输出会按比例偏移。

### 假设 3: ISP 处理瓶颈

ISP 已启用 (`CONFIG_ESP_VIDEO_ENABLE_ISP=y`), RAW8 Bayer 数据需经过 ISP demosaicing 处理。
如果 ISP 处理缓慢, 会限制帧率。

## 7. 下一步行动 (优先级排序)

### P0: 验证并修复 skip_frames 问题

1. 将 `CAMERA_FPS` 从 30 改为 **50** (50 是 sensor fps 的因数):
   ```c
   #define CAMERA_FPS  50  // 原来是 30
   ```
   这样 `denominator=50`, `50 % 50 == 0`, S_PARM 会成功
   `skip_frames = 50/50 = 1` (不跳帧)

2. 或者直接传 50fps:
   ```c
   set_stream_fps(fd, 50);
   ```

### P1: 如果 P0 无效, 检查 XCLK

1. 尝试不同的 XCLK 频率 (12MHz, 16MHz)
2. 或改用 ESP clock router 提供 XCLK (匹配 sdkconfig)

### P2: 如果 P1 无效, 排查 ISP

1. 在 menuconfig 中禁用 ISP: `CONFIG_ESP_VIDEO_ENABLE_ISP=n`
2. 或直接请求 YUYV/RGB565 格式绕过 ISP

## 8. 关键文件位置

| 文件 | 路径 |
|------|------|
| 主程序 | `main/CameraCtrl.c` |
| sdkconfig | `sdkconfig` |
| OV5647 驱动 | `managed_components/espressif__esp_cam_sensor/sensors/ov5647/ov5647.c` |
| OV5647 寄存器表 | `managed_components/espressif__esp_cam_sensor/sensors/ov5647/private_include/ov5647_settings.h` |
| OV5647 头文件 | `managed_components/espressif__esp_cam_sensor/sensors/ov5647/include/ov5647.h` |
| CSI 视频驱动 | `managed_components/espressif__esp_video/src/device/esp_video_csi_device.c` |
| Video 核心 | `managed_components/espressif__esp_video/src/esp_video.c` |
| PC 接收端 | `pc_receiver.py` |

## 9. 配置要点

```
CONFIG_CAMERA_OV5647_MIPI_RAW8_800X640_50FPS=y
CONFIG_CAMERA_OV5647_CSI_LINESYNC_ENABLE=y
CONFIG_CAMERA_XCLK_USE_ESP_CLOCK_ROUTER=y
CONFIG_ESP_VIDEO_ENABLE_ISP=y
CONFIG_ESP_VIDEO_DISABLE_MIPI_CSI_DRIVER_BACKUP_BUFFER=y
CONFIG_ESP_HOSTED_CP_TARGET_ESP32C6=y
```

## 10. Git 状态

- 分支: main
- 最近提交: `2f32f73 feat: 成功获取视频帧`
- 本地修改: `main/CameraCtrl.c`, `pc_receiver.py`, `sdkconfig`
