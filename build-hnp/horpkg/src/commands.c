#include "commands.h"
#include "utils.h"
#include <stdio.h>

int cmd_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Horpkg - Local HAP Installer         ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");

    printf("%s用法:%s horpkg <command> [options]\n\n", COLOR_BOLD, COLOR_RESET);
    printf("%s可用命令:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %sinit%s                设备初始化，获取并保存设备信息/凭证\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sinstall%s <本地hap>    安装本地 HAP/HSP 到已连接设备（包含签名）\n", COLOR_GREEN, COLOR_RESET);
    printf("  %shelp%s                 显示帮助信息\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sversion%s              显示版本\n", COLOR_GREEN, COLOR_RESET);
    printf("\n");
    printf("请先通过 HDC 连接设备，再执行 install。\n");
    printf("\n");
    return 0;
}

int cmd_version(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("\n");
    printf("%sHorpkg%s v%s\n", COLOR_BOLD, COLOR_RESET, HORPKG_VERSION);
    printf("本地 HAP 安装工具\n");
    printf("\n");
    return 0;
}
