/**
 *******************************************************************************
 * @file    cmd-file.h
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

#pragma once



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../app-file.h"
#include "../app-shell.h"




/*-------- 2. enum ---------------------------------------------------------------------------------------------------*/


/*-------- 3. interface ---------------------------------------------------------------------------------------------*/


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/


/*-------- 5. factories ----------------------------------------------------------------------------------------------*/

int cat(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), cat, cat, display the file);

int echo(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), echo, echo, write into the file);

int touch(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN), touch, touch, create new file);

int ls(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),ls, ls, display the direction);

int cd(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),cd, cd, change direction);

int mkdir(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),mkdir, mkdir, make dir);

int rm(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),rm, rm, remove file or direction);

int move(int argc, char* argv[]);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),move, move, move the file or dirction);