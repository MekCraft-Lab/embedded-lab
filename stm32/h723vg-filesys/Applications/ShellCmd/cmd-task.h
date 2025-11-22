/**
 *******************************************************************************
 * @file    task.h
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
 * @date    2025/8/26
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/


/*-------- includes --------------------------------------------------------------------------------------------------*/

#include "../app-shell.h"


/*-------- typedef ---------------------------------------------------------------------------------------------------*/


/*-------- define ----------------------------------------------------------------------------------------------------*/


/*-------- macro -----------------------------------------------------------------------------------------------------*/


/*-------- variables -------------------------------------------------------------------------------------------------*/


/*-------- function prototypes ---------------------------------------------------------------------------------------*/

int ps(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), ps, ps, display the task infomation);

int top(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), top, top, display the CPU comsume);

int osFree(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), osfree, osFree,
                 display the remain heap size);

int stack(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), stack, stack,
                 display the minimum stack size of task);