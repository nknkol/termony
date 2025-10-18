#ifndef HORPKG_COMMANDS_H
#define HORPKG_COMMANDS_H

// 命令处理函数
int cmd_init(int argc, char *argv[]);
int cmd_install(int argc, char *argv[]);
int cmd_remove(int argc, char *argv[]);
int cmd_update(int argc, char *argv[]);
int cmd_list(int argc, char *argv[]);
int cmd_search(int argc, char *argv[]);
int cmd_info(int argc, char *argv[]);
int cmd_sync(int argc, char *argv[]);
int cmd_clean(int argc, char *argv[]);
int cmd_config(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_version(int argc, char *argv[]);

#endif // HORPKG_COMMANDS_H