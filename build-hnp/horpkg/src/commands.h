#ifndef HORPKG_COMMANDS_H
#define HORPKG_COMMANDS_H

#include "hdc.h"

// 命令处理函数
int cmd_init(int argc, char *argv[]);
int cmd_install(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_version(int argc, char *argv[]);

#endif // HORPKG_COMMANDS_H
