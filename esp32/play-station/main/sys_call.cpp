/**
 *******************************************************************************
 * @file    sys_call.cpp
 * @brief   模拟器需要的系统调用
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

#define  DEFAULT_SAMPLERATE   22100
#define  DEFAULT_FRAGSIZE     128

/* ------- include ---------------------------------------------------------------------------------------------------*/


#include "applications/app-display.h"
#include "esp_lcd_panel_commands.h"
#include "esp_log.h"
#include "lcd-i80.h"
#include "osd.h"


#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <log.h>
#include <nofconfig.h>
#include <nofrendo.h>
#include <noftypes.h>

#include <version.h>


/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/




/* ------- function implement ----------------------------------------------------------------------------------------*/

/* vim: set tabstop=3 expandtab:
**
** This file is in the public domain.
**
** osd.c
**
** $Id: osd.c,v 1.2 2001/04/27 14:37:11 neil Exp $
**
*/


char configfilename[] = "na";

/* This is os-specific part of main() */
extern "C" int osd_main(int argc, char* argv[]) {
    config.filename = configfilename;

    return main_loop("/littlefs/Super Mario Bros.NES", system_autodetect);
}

/* File system interface */
extern "C" void osd_fullname(char* fullname, const char* shortname) { strncpy(fullname, shortname, PATH_MAX); }

/* This gives filenames for storage of saves */
extern "C" char* osd_newextension(char* string, char* ext) { return string; }

/* This gives filenames for storage of PCX snapshots */
extern "C" int osd_makesnapname(char* filename, int len) { return -1; }


/**
 * @brief 获取系统播放函数
 * @param playfunc 配置播放函数
 */
extern "C" void osd_setsound(void (*playfunc)(void* buffer, int size)) {}


/**
 * @brief 获取系统显示信息配置
 * @param info 显示信息配置
 */
extern "C" void osd_getvideoinfo(vidinfo_t* info) {

    static viddriver_t driver = {
        .name        = "st7789v-esp32p4-lvgl",
        .init        = vidInit,
        .shutdown    = vidShutdown,
        .set_mode    = vidSetMode,
        .set_palette = vidSetPalette,
        .clear       = vidClear,
        .lock_write  = vidLockWrite,
        .free_write = vidFreeWrite,
        .custom_blit = vidBlit,
        .invalidate = false,
    };

    static vidinfo_t temp = {
        .default_width  = DISP_WIDTH,
        .default_height = DISP_HEIGHT,
        .driver         = &driver,
    };

    *info = temp;
}

/**
 * @brief 获取系统声音输出信息
 * @param info 声音信息
 */
extern "C" void osd_getsoundinfo(sndinfo_t *info) {

    static sndinfo_t temp = {
        .sample_rate = DEFAULT_SAMPLERATE,
        .bps = DEFAULT_FRAGSIZE,
    };

    *info = temp;

}

/**
 * @brief OSD初始化接口
 * @return 0
 */
extern "C" int osd_init()
{

    log_chain_logfunc(logout);

    return 0;
}


/**
 * @brief 系统关机
 */
extern "C" void osd_shutdown() {
    ESP_LOGI("NES-LOG", "system shutdown");

}



static void (*nofrendoTimerCallback)();
void nofrendoTimer(TimerHandle_t timerHandler) {
    nofrendoTimerCallback();
}
//Seemingly, this will be called only once. Should call func with a freq of frequency,
extern "C" int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
    nofrendoTimerCallback = (void(*)())func;

    printf("Timer install, freq=%d\n", frequency);
    TimerHandle_t timer=xTimerCreate("nes",configTICK_RATE_HZ/frequency, pdTRUE, NULL, nofrendoTimer);
    xTimerStart(timer, 0);
    return 0;
}

extern "C" char *osd_getromdata(const char* filename) {

    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Failed to open ROM file: %s\n", filename);
        return nullptr;
    }

    // 获取文件大小
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // 必须分配可移动的可写内存（Nofrendo 会修改指针 rom）
    char* buf =
        (char*)malloc(size);

    if (!buf) {
        printf("Failed to alloc ROM buffer: %u bytes\n", size);
        fclose(f);
        return nullptr;
    }

    fread(buf, 1, size, f);
    fclose(f);

    printf("ROM loaded to %p, size=%u\n", buf, size);

    return buf;
}



extern "C" void osd_getmouse(int *x, int *y, int *button){}