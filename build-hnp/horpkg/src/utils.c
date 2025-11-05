#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>


static void trim_whitespace(char *str) {
    print_info("[LOG] trim_whitespace: Entered"); // <-- 新日志
    if (str == NULL) {
        print_info("[LOG] trim_whitespace: str is NULL, returning"); // <-- 新日志
        return;
    }

    // 1. 查找第一个非空白字符
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    print_info("[LOG] trim_whitespace: Found first non-space"); // <-- 新日志

    // 2. 将非空白部分（包括 \0）移动到字符串开头
    if (start != str) {
        // strlen(start) + 1 确保 \0 终止符也被复制
        memmove(str, start, strlen(start) + 1);
        print_info("[LOG] trim_whitespace: memmove complete"); // <-- 新日志
    }

    // 3. 移除尾部的空白字符
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        len--;
    }
    str[len] = '\0'; // 设置新的 \0 终止符
    print_info("[LOG] trim_whitespace: Trailing whitespace removed, exiting"); // <-- 新日志
}
// --- 修复结束 ---

void print_success(const char *msg) {
    printf("%s✓%s %s\n", COLOR_GREEN, COLOR_RESET, msg);
}

void print_error(const char *msg) {
    fprintf(stderr, "%s✗ Error:%s %s\n", COLOR_RED, COLOR_RESET, msg);
}

void print_warning(const char *msg) {
    printf("%s⚠ Warning:%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

void print_info(const char *msg) {
    printf("%s→%s %s\n", COLOR_BLUE, COLOR_RESET, msg);
}

void print_prompt(const char *msg) {
    printf("%s?%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

// ← 添加格式化版本的函数
void print_error_fmt(const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    print_error(buffer);
}

void print_warning_fmt(const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    print_warning(buffer);
}

void print_info_fmt(const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    print_info(buffer);
}

void print_success_fmt(const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    print_success(buffer);
}

int create_dir_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to create directory %s: %s", path, strerror(errno));
            print_error(err_msg);
            return -1;
        }
    }
    return 0;
}

char* get_config_path(const char* filename) {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        print_error("HOME environment variable not set.");
        return NULL;
    }

    char base_path[256];
    snprintf(base_path, sizeof(base_path), "%s/.horpkg", home_dir);
    if (create_dir_if_not_exists(base_path) != 0) {
         return NULL; 
    }
    
    // 使用 snprintf 保证安全
    size_t len = strlen(home_dir) + strlen("/.horpkg/") + strlen(filename) + 1;
    char* path = malloc(len);
    if (path) {
        snprintf(path, len, "%s/.horpkg/%s", home_dir, filename);
    }
    
    return path;
}

// --- HDC 功能函数 (使用 hdc-lite) ---

/**
 * @brief 尝试启动HDC服务 (system call)
 */
void hdc_start_service(void) {
    // --- (REQ 2): 添加日志 ---
    print_info("Ensuring HDC service is running...");
    // 执行 "hdc-lite start"，重定向输出避免污染 stdout
    system("hdc-lite start > /dev/null 2>&1");
    print_info("HDC service check complete.");
    // --- END MODIFIED ---
}

/**
 * @brief 检查HDC连接状态 (增加了日志)
 * @return 1 表示已连接, 0 表示未连接
 */
int hdc_is_connected(void) {
    print_info("[LOG] hdc_is_connected: Entered function"); // <-- 新日志
    char line[256];
    FILE *fp = popen("hdc-lite list targets", "r");
    if (fp == NULL) {
        print_error("Failed to run 'hdc-lite'. Is it in your PATH?");
        print_info("[LOG] hdc_is_connected: popen failed, returning 0"); // <-- 新日志
        return 0; 
    }
    print_info("[LOG] hdc_is_connected: popen() successful"); // <-- 新日志

    // 读取第一行输出
    if (fgets(line, sizeof(line), fp) == NULL) {
        print_info("[LOG] hdc_is_connected: fgets() returned NULL (no output)"); // <-- 新日志
        pclose(fp);
        print_info("[LOG] hdc_is_connected: pclose() after fgets NULL, returning 0"); // <-- 新日志
        return 0; // 没有输出
    }
    print_info("[LOG] hdc_is_connected: fgets() successful"); // <-- 新日志
    
    pclose(fp);
    print_info("[LOG] hdc_is_connected: pclose() successful"); // <-- 新日志

    trim_whitespace(line);
    print_info("[LOG] hdc_is_connected: trim_whitespace() complete"); // <-- 新日志

    // 打印修剪后的行内容
    printf("[LOG] Trimmed line: \"%s\"\n", line); // <-- 新日志

    // 如果输出是 "[Empty]" 或空字符串，则未连接
    if (strcmp(line, "[Empty]") == 0 || strlen(line) == 0) {
        print_info("[LOG] hdc_is_connected: Result: Not connected (0)"); // <-- 新日志
        return 0;
    }

    // 否则，假定已连接
    print_info("[LOG] hdc_is_connected: Result: Connected (1)"); // <-- 新日志
    return 1;
}

/**
 * @brief 尝试连接到指定的HDC端口 (Functional)
 * @param port 端口号 (e.g., "38201")
 * @return 0 表示命令执行成功, -1 表示失败
 */
int hdc_connect_port(const char *port) {
    char command[256];
    snprintf(command, sizeof(command), "hdc-lite tconn 127.0.0.1:%s > /dev/null 2>&1", port);
    
    print_info("Executing connection command...");
    int status = system(command);
    if (status != 0) {
        print_error("Failed to execute 'hdc-lite tconn' command.");
        return -1;
    }
    
    usleep(500000);
    return 0;
}

/**
 * @brief 通过HDC获取设备UUID (Functional)
 * @return 成功则返回UUID字符串 (需要free), 失败返回NULL
 */
char* hdc_get_uuid(void) {
    FILE *fp;
    char line[256];
    
    fp = popen("hdc-lite shell bm get --udid", "r");
    if (fp == NULL) {
        print_error("Failed to run hdc-lite command");
        return NULL;
    }

    char* uuid = NULL;
    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_whitespace(line);
        if (strlen(line) == 64) {
            uuid = malloc(strlen(line) + 1); 
            if (uuid) {
                strcpy(uuid, line);
            }
            break;
        }
    }
    pclose(fp);
    
    return uuid; // 如果没找到，将返回 NULL
}
/**
 * @brief 检查horpkg是否已初始化 (是否已有UUID，并且UUID是否与当前设备匹配)
 * @return 1 表示已初始化且设备匹配, 0 表示未初始化或设备不匹配
 */
int is_initialized(void) {
    if (g_config.device_uuid[0] == '\0') {
        print_info("[LOG] is_initialized: device_uuid in config is empty.");
        return 0;
    }

    char* current_uuid = hdc_get_uuid();
    if (current_uuid == NULL) {
        print_info("[LOG] is_initialized: Could not get current UUID from HDC (device disconnected?).");
        return 0; 
    }

    int match = (strcmp(g_config.device_uuid, current_uuid) == 0);
    
    if (match) {
        print_info("[LOG] is_initialized: Stored UUID matches current device.");
    } else {
        char truncated_stored[11] = {0};
        char truncated_current[11] = {0};
        strncpy(truncated_stored, g_config.device_uuid, 10);
        strncpy(truncated_current, current_uuid, 10);
        
        print_warning_fmt("Device mismatch: Initialized for %s..., but current device is %s...", 
                          truncated_stored, truncated_current);
        print_prompt("Please run 'horpkg init' to re-initialize for this device.");
    }

    free(current_uuid);
    return match;
}

int store_uuid(const char* uuid) {
    if (!uuid) return -1;
    
    strncpy(g_config.device_uuid, uuid, sizeof(g_config.device_uuid) - 1);
    g_config.device_uuid[sizeof(g_config.device_uuid) - 1] = '\0';
    
    if (config_save() != 0) {
        print_error("Failed to save UUID to config.json");
        return -1;
    }
    return 0;
}