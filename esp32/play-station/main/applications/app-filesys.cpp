/**
 *******************************************************************************
 * @file    app-filesys.cpp
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/12/12
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ----------------------------------------------------------------------------------------------------*/






/* ------- include ---------------------------------------------------------------------------------------------------*/



/* I. header */

#include "app-filesys.h"

#include "esp_littlefs.h"
#include "esp_log.h"
#include "sys/stat.h"
/* II. other application */


/* III. standard lib */
#include <dirent.h>



/* ------- class prototypes-----------------------------------------------------------------------------------------*/




/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- application attribute -------------------------------------------------------------------------------------*/

#define APPLICATION_ENABLE     true

#define APPLICATION_NAME       "Filesys"

#define APPLICATION_STACK_SIZE 8192

#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];





/* ------- message interface attribute -------------------------------------------------------------------------------*/






/* ------- function prototypes ---------------------------------------------------------------------------------------*/





/* ------- fnction implement ----------------------------------------------------------------------------------------*/


/* ------------ LittleFS 初始化 ------------ */
static esp_err_t init_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(APPLICATION_NAME, "Failed to mount LittleFS (%s)", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(APPLICATION_NAME, "LittleFS mounted successfully");

    size_t total = 0, used = 0;
    ret = esp_littlefs_info("littlefs", &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(APPLICATION_NAME, "FS size: total=%d KB, used=%d KB", total / 1024, used / 1024);
    }

    return ESP_OK;
}


FilesysApp::FilesysApp()
    : StaticAppBase(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE,  appStack, APPLICATION_PRIORITY, 0, nullptr){
}


FilesysApp& FilesysApp::instance() {
    static FilesysApp instance;
    return instance;
}

uint8_t fileBuf[128] = {0};
void FilesysApp::init() {
    /* driver object initialize */

    _filesysSemaphore = xSemaphoreCreateBinary();

    init_littlefs();


    if (readFile("head.txt", fileBuf, sizeof(fileBuf))) {
        printf("读到文件内容：%s\n", fileBuf);
    } else {
        ESP_LOGE(APPLICATION_NAME, "Failed to read file");
    }

    listFiles(nullptr);

    xSemaphoreGive(_filesysSemaphore);

}


void FilesysApp::run() {
    vTaskDelay(1);
}


bool FilesysApp::readFile(const char* path, void* dataout, size_t max_len) {
    if (!_filesysSemaphore) {
        return false;
    }

    if (xSemaphoreTake(_filesysSemaphore, pdMS_TO_TICKS(200)) != pdTRUE)
        return false;

    char fullpath[128];
    snprintf(fullpath, sizeof(fullpath), "/%s", path);

    FILE *f = fopen(fullpath, "r");
    if (!f) {
        ESP_LOGE(APPLICATION_NAME, "Failed to open %s", fullpath);
        xSemaphoreGive(_filesysSemaphore);
        return false;
    }

    memset(dataout, 0, max_len);
    fread(dataout, 1, max_len - 1, f);
    fclose(f);

    xSemaphoreGive(_filesysSemaphore);
    return true;


}

char path[512];
bool FilesysApp::listFiles(const char* dirpath) {
    const char *base_path = "/littlefs";
    DIR *dir = opendir(base_path);
    if (!dir) {
        ESP_LOGE("Filesys", "Failed to open directory %s", base_path);
        return false;
    }

    struct dirent *entry;
    struct stat st;

    ESP_LOGI("Filesys", "Listing files in %s:", base_path);

    while ((entry = readdir(dir)) != NULL) {

        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

        if (stat(path, &st) == 0) {
            ESP_LOGI("Filesys", "  %s   (%ld bytes)", entry->d_name, st.st_size);
        } else {
            ESP_LOGW("Filesys", "  %s   (stat failed)", entry->d_name);
        }
    }

    return true;
}

uint8_t FilesysApp::rxMsg(void* msg, uint16_t size) {

    return 0;
}

uint8_t FilesysApp::rxMsg(void* msg, uint16_t size, TickType_t timeout) {

    return 0;
}

