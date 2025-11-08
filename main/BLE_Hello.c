#include <esp_bt.h>
#include <host/ble_gap.h>
#include <host/ble_hs.h>
#include <nvs_flash.h>
#include <stdio.h>

#include <host/ble_gatt.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>

#include "esp_nimble_hci.h"
#include <os/os_mbuf.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

static const char* TAG = "BLE_HELLO";

#define MAX_INPUT_REPORT 8
static uint16_t report_handles[MAX_INPUT_REPORT];
static uint16_t cccd_handles[]  = {40, 44, 48, 52};
static int input_report_count   = 0;
static int cccd_write_index     = 0;
static uint8_t cccd_queue_index = 0; // 当前队列索引


static int gap_event_cb(struct ble_gap_event* event, void* arg);
/* --------------------- 特征值回调 -----------------------*/
/**
 * @brief 当客户端对characteristic 进行 read / write 时，
 * NimBLE 会调用这个函数
 * 这里只响应read操作，返回字符串 "Hello World!"
 * @param conn_handle 连接句柄，标识来自哪个客户端
 * @param attr_handle 内部表项句柄
 * @param ctxt 上下文，请求类型(ctxt->op)与数据缓冲区(ctxt->om)
 * @param arg 注册时传入自定义指针
 * @return int
 */
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt,
                               void* arg) {
    // 判断是不是READ操作
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        const char* hello = "Hello World!";
        size_t len        = strlen(hello);

        /*
         * 将要返回的数据写入response的mbuf (ctxt->om)中。
         * os_mbuf_append函数会将数据追加到发送的缓冲区中
         */

        int rc            = os_mbuf_append(ctxt->om, hello, len);

        if (rc != 0) {
            ESP_LOGE(TAG, "os_mbuf_append failed: %d", rc);
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        ESP_LOGI(TAG, "GATT_ACCESS_OP_READ_CHR returned: %d", rc);
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/* --------------------- GATT 服务定义 --------------------*/
/**
 * @brief
 * - NimBLE 使用 struct ble_gatt_svc_def 数组来描述整个GATT数据库(服务->特征->描述符)
 * - 这个结构体在程序启动时被注册(ble_gatts_count_cfg/ ble_gatts_add_svcs)
 * - 此处创建一个服务(UUID使用16-bit)，服务内含一个可读特征
 */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {/* 类型：主服务(Primary Service) */
     .type = BLE_GATT_SVC_TYPE_PRIMARY,
     /* 服务UUID (这里使用Device Information Service UUID 作为示例,
      * 关键是特征自定义
      */
     .uuid = BLE_UUID16_DECLARE(0x180A), // 设备信息服务

     /* 特征列表（终止项为{0}） */
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {
                 .uuid      = BLE_UUID16_DECLARE(0x2A57), // 自定义特征
                 .access_cb = gatt_svr_chr_access,        // 访问回调
                 .flags     = BLE_GATT_CHR_F_READ,        // 权限定义，可读
             },
             {0} // 特征结束
         }},
    {0} // 服务结束
};


static int write_cccd(uint16_t conn_handle, const struct ble_gatt_error* error, uint16_t chr_val_handle,
                      const struct ble_gatt_dsc* dsc, void* arg) {
    if (!dsc) return 0;  // 避免空指针

    if (dsc->uuid.u.type == BLE_UUID_TYPE_16 && dsc->uuid.u16.value == 0x2902) {
        ESP_LOGI(TAG, "找到CCCD, handle=%u", dsc->handle);

        uint8_t notify_value[2] = {0x01, 0x00}; // 0x01=Notify, 0x02=Indicate
        int rc2 = ble_gattc_write_flat(conn_handle, dsc->handle, notify_value, sizeof(notify_value), NULL, NULL);
        if (rc2 == 0) {
            ESP_LOGI(TAG, "Notify已启用");
        } else {
            ESP_LOGE(TAG, "写CCCD失败: %d", rc2);
        }
    }
    return 0; // 返回0继续遍历
}

void enable_notify(uint16_t conn_handle, uint16_t char_handle) {
    int rc;

    // 遍历特征描述符
    rc = ble_gattc_disc_all_dscs(conn_handle, char_handle, 62, write_cccd, NULL);

    if (rc != 0) {
        ESP_LOGE(TAG, "发现描述符失败: %d", rc);
    }
}
/* --- 特征发现回调 --- */
static int hid_chr_disc_cb(uint16_t conn_handle, const struct ble_gatt_error* error, const struct ble_gatt_chr* chr,
                           void* arg) {

    if (error->status != 0) {
        printf("Discover characteristic failed: %d\n", error->status);
    }

    if (chr == NULL) {
        // 所有特征发现完成
        printf("特征发现完成\n");
        cccd_queue_index = 0;
        enable_notify(conn_handle, 0x01);

        // 使能Notify

        uint8_t data[2]  = {0x00, 0x01}; // Notify = 0x0001（小端）
        struct os_mbuf* om;

        om     = ble_hs_mbuf_from_flat(data, sizeof(data));

        int rc = ble_gattc_write_no_rsp_flat(conn_handle, cccd_handles[0], om, sizeof(data));

        if (rc != 0) {
            printf("CCCD write failed rc=%d\n", rc);
        } else {
            printf("CCCD write success, handle=%u\n", cccd_handles[0]);
        }

        return 0;
    }

    // 发现 Input Report 特征 (UUID 0x2A4D)
    if (chr->uuid.u16.value == 0x2A4D) {
        if (input_report_count < MAX_INPUT_REPORT) {
            // 保存句柄
            report_handles[input_report_count] = chr->val_handle;
            input_report_count++;
            printf("Report #%d handle=%u\n", input_report_count, chr->val_handle);
        }
    }

    return 0;
}


static int gatt_service_cb(uint16_t conn_handle, const struct ble_gatt_error* err, const struct ble_gatt_svc* svc,
                           void* arg) {
    if (svc == NULL)
        return 0;

    // 找到HID服务
    if (svc->uuid.u.type == BLE_UUID_TYPE_16 && svc->uuid.u16.value == 0x1812) {
        printf("找到 HID 服务，范围 %u ~ %u\n", svc->start_handle, svc->end_handle);
        static uint32_t handle_range[2];
        handle_range[0] = svc->start_handle;
        handle_range[1] = svc->end_handle;

        // 下一步：查找特征
        ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle, hid_chr_disc_cb, handle_range);
    }

    return 0;
}


// per-connection 回调函数
static int per_conn_cb(struct ble_gap_event* event, void* arg) {
    uint16_t conn_handle = event->connect.conn_handle;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "[%u] Connection established", conn_handle);
            } else {
                ESP_LOGE(TAG, "[%u] Connection failed, status=%d", conn_handle, event->connect.status);
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "[%u] Disconnected, reason=0x%02X", conn_handle, event->disconnect.reason);
            break;

        case BLE_GAP_EVENT_NOTIFY_RX: {
            static uint8_t cnt;
            if (cnt++ % 23 == 0) {
                const struct os_mbuf* om = event->notify_rx.om;
                uint16_t handle          = event->notify_rx.attr_handle;

                uint8_t buf[256];
                int len = om->om_len > sizeof(buf) ? sizeof(buf) : om->om_len;
                os_mbuf_copydata(om, 0, len, buf);

                int16_t left_x_temp = buf[0] | (buf[1] << 8);
                int16_t left_y_temp = buf[2] | (buf[3] << 8);
                int16_t right_x_temp = buf[4] | (buf[5] << 8);
                int16_t right_y_temp = buf[6] | (buf[7] << 8);

                printf("%d-%d %d-%d | ", left_x_temp, left_y_temp, right_x_temp, right_y_temp);

                for (int i = 8; i < len; i++) {
                    printf("%02X ", buf[i]);
                }
                printf("\n");
            }
            break;
        }

        case BLE_GAP_EVENT_SUBSCRIBE: {
            ESP_LOGI(TAG, "[%u] Subscribe event: conn_handle=%u, attr_handle=%u, reason=%u", conn_handle,
                     event->subscribe.conn_handle, event->subscribe.attr_handle, event->subscribe.reason);
            break;
        }

        default:
            ESP_LOGI(TAG, "[%u] Unknown GAP event: %d", conn_handle, event->type);
            break;
    }

    return 0;
}


static int gap_event_cb(struct ble_gap_event* event, void* arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connection established");
                uint16_t conn_handle = event->connect.conn_handle;


                ble_gap_set_event_cb(conn_handle, per_conn_cb, NULL);

                // 发现服务
                ble_gattc_disc_svc_by_uuid(conn_handle, BLE_UUID16_DECLARE(0x1812), gatt_service_cb, NULL);
            } else {
                ESP_LOGE(TAG, "GAP event not handled");
            }
            break;

        case BLE_GAP_EVENT_DISC: {

            const uint8_t target_mac[] = {0xE0, 0x4E, 0x24, 0x8D, 0xED, 0xB2};
            struct ble_hs_adv_fields fields;

            ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);

            for (uint8_t i = 0; i < 6; i++) {
                if (event->disc.addr.val[i] != target_mac[5 - i]) {
                    return 0;
                }
            }
            ESP_LOGI(TAG, "发现设备 ");
            // 解析广播名称
            {
                struct ble_hs_adv_fields fields;
                if (ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data) == 0) {

                    if (fields.name_len > 0) {
                        char name[32] = {0};
                        memcpy(name, fields.name, fields.name_len);
                        ESP_LOGI(TAG, "Name: %s", name);
                    }
                }
            }
            ESP_LOGI(TAG, "MAC : %02X:%02X:%02X:%02X:%02X:%02X\n", event->disc.addr.val[5], event->disc.addr.val[4],
                     event->disc.addr.val[3], event->disc.addr.val[2], event->disc.addr.val[1],
                     event->disc.addr.val[0]);

            ESP_LOGI(TAG, "找到手柄，开始连接");
            ble_gap_disc_cancel();

            int rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC,
                                     &event->disc.addr, // 目标设备地址
                                     30000,             // 超时
                                     NULL,              // 默认连接参数
                                     gap_event_cb,      // 连接回调
                                     NULL);

            ESP_LOGI(TAG, "连接返回值：%d", rc);
        } break;

        default:
            ESP_LOGI(TAG, "Unknown event");
    }

    return 0;
}

static void parse_name_from_adv(const uint8_t* adv_data, uint8_t adv_len) {
    uint8_t index = 0;

    while (index < adv_len) {
        uint8_t length = adv_data[index];
        if (length == 0) {
            break;
        }

        uint8_t type = adv_data[index + 1];

        /** Complete Local Name */
        if (type == 0x09) {
            printf("Device Name: %.*s\n", length - 1, &adv_data[index + 2]);
            return;
        }

        /** Shortened Local Name */
        if (type == 0x08) {
            printf("Short Name: %.*s\n", length - 1, &adv_data[index + 2]);
            return;
        }

        index += length + 1;
    }
}

static void start_scan(void) {
    struct ble_gap_disc_params params = {0};

    params.itvl                       = 0x20; // 扫描间隙
    params.window                     = 0x10; // 扫描窗口
    params.passive                    = 0x0;  // 主动扫描

    ESP_LOGI(TAG, "Starting scan");

    // 注册回调
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &params, gap_event_cb, NULL);
}

/* ------------------------ 同步完成回调 ---------------------------*/
/* NimBLE Host 与 Controller 完成同步
 * 主机可以开始BLE功能, NimBLE会调用这个回调
 *
 * 回调中要：
 *  推断设备地址类型(ble_hs_id_infer_auto)
 *  启动广播(ble_app_adverise)
 */

static void ble_app_on_sync(void) {
    ESP_LOGI(TAG, "BLE Host synced");

    // -------- [关键] 如果没有地址，生成随机静态地址 --------
    uint8_t addr_val[6];
    int rc;

    rc = ble_hs_id_infer_auto(0, &addr_val[0]);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to infer address, rc=%d", rc);
        return;
    }

    char addr_str[18];
    snprintf(addr_str, sizeof(addr_str), "%02X:%02X:%02X:%02X:%02X:%02X", addr_val[5], addr_val[4], addr_val[3],
             addr_val[2], addr_val[1], addr_val[0]);
    ESP_LOGI(TAG, "Device address: %s", addr_str);

    start_scan();
}

/* ======================== 6) NimBLE 栈重置回调（可选，但推荐） ==============
 *
 * 如果 NimBLE 因某种原因被复位，会调用这个回调。
 * 你可在这里打印原因、做清理或重启相关功能。
 */
static void ble_app_on_reset(int reason) { ESP_LOGE(TAG, "NimBLE reset; reason=%d", reason); }

/* --------------------------- Host task 启动函数 -------------------------- */
/*
 * nimble_port_freertos_init()需要一个函数指针，host task的函数
    该函数调用 nimble_port_run() 运行NimBLE事件循环
 */
static void nimble_host_task(void* param) {
    ESP_LOGI(TAG, "Starting host");
    nimble_port_run();
    vTaskDelete(NULL);
}

/* ---------------------- 主程序入口 ---------------------------*/
/*
*
* 初始化NVS
    启动蓝牙控制器
    初始化NimBLE Host移植层
    初始化GAP/ GATT 服务 ble_svc_gap_init/ ble_svc_gatt_init
    注册 GATT服务表 - ble_gatts_count_cfg/ ble_gatts_add_svcs
    *设置NimBLE 回调 - ble_hs_cfg
    *启动NimBLE 主任务 - nimble_port_freertos_init
 */

void app_main(void) {
    esp_err_t ret;
    /* 1. NVS初始化 */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 2. 初始化底层蓝牙控制器外设 */
    nimble_port_init();

    /* 3. 注册GATT服务 */
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    /* 4. 设置同步回调 */
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    /* 5. 启动 NimBLE Host 线程 */
    nimble_port_freertos_init(nimble_host_task);
}
