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
#include <limits.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include "logger.h"

static void canonicalize_path_inplace(char *path, size_t buffer_size) {
    if (!path || buffer_size == 0 || path[0] == '\0') {
        return;
    }
#if defined(PATH_MAX)
    char resolved[PATH_MAX];
    if (realpath(path, resolved)) {
        strncpy(path, resolved, buffer_size - 1);
        path[buffer_size - 1] = '\0';
        return;
    }
#endif
    (void)buffer_size;
}

void print_error_fmt(const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    // [修改] 调用 print_error，它已被重定向到 logger
    print_error(buffer);
}

void print_warning_fmt(const char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    // [修改] 调用 print_warning
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

static void trim_whitespace(char *str) {
    log_debug("trim_whitespace: Entered");
    if (str == NULL) {
        log_debug("trim_whitespace: str is NULL, returning");
        return;
    }

    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    log_debug("trim_whitespace: Found first non-space");

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
        log_debug("trim_whitespace: memmove complete");
    }

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        len--;
    }
    str[len] = '\0';
    log_debug("trim_whitespace: Trailing whitespace removed, exiting");
}

void print_success(const char *msg) {
    logger_log(LOG_LEVEL_INFO, NULL, 0, "%s✓%s %s", COLOR_GREEN, COLOR_RESET, msg);
}

void print_error(const char *msg) {
    logger_log(LOG_LEVEL_WARN, NULL, 0, "%s✗ Error:%s %s", COLOR_RED, COLOR_RESET, msg);
}

void print_warning(const char *msg) {
    logger_log(LOG_LEVEL_WARN, NULL, 0, "%s⚠ Warning:%s %s", COLOR_YELLOW, COLOR_RESET, msg);
}

void print_info(const char *msg) {
    logger_log(LOG_LEVEL_INFO, NULL, 0, "%s→%s %s", COLOR_BLUE, COLOR_RESET, msg);
}

void print_prompt(const char *msg) {
    printf("%s?%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

int create_dir_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to create directory %s: %s", path, strerror(errno));
            print_error_fmt("%s", err_msg);
            return -1;
        }
    }
    return 0;
}

static char* build_horpkg_path(const char *subdir, const char *filename) {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        print_error("HOME environment variable not set.");
        return NULL;
    }

    size_t base_len = strlen(home_dir) + strlen("/.horpkg") + 1;
    char *base_dir = malloc(base_len);
    if (!base_dir) {
        print_error("Failed to allocate memory for base configuration path.");
        return NULL;
    }
    snprintf(base_dir, base_len, "%s/.horpkg", home_dir);

    if (create_dir_if_not_exists(base_dir) != 0) {
        free(base_dir);
        return NULL;
    }

    char *target_dir = NULL;
    if (subdir && subdir[0] != '\0') {
        size_t target_len = strlen(base_dir) + 1 + strlen(subdir) + 1;
        target_dir = malloc(target_len);
        if (!target_dir) {
            print_error("Failed to allocate memory for configuration subdirectory.");
            free(base_dir);
            return NULL;
        }
        snprintf(target_dir, target_len, "%s/%s", base_dir, subdir);
        if (create_dir_if_not_exists(target_dir) != 0) {
            free(base_dir);
            free(target_dir);
            return NULL;
        }
    } else {
        target_dir = strdup(base_dir);
        if (!target_dir) {
            print_error("Failed to duplicate base directory path.");
            free(base_dir);
            return NULL;
        }
    }
    free(base_dir);

    const char *name = filename ? filename : "";
    size_t final_len = strlen(target_dir) + (name[0] ? 1 + strlen(name) : 0) + 1;
    char *full_path = malloc(final_len);
    if (!full_path) {
        print_error("Failed to allocate memory for configuration path.");
        free(target_dir);
        return NULL;
    }

    if (name[0]) {
        snprintf(full_path, final_len, "%s/%s", target_dir, name);
    } else {
        snprintf(full_path, final_len, "%s", target_dir);
    }

    free(target_dir);
    return full_path;
}

char* get_config_path(const char* filename) {
    return build_horpkg_path(NULL, filename);
}

char* get_signature_path(const char* filename) {
    return build_horpkg_path("signature", filename);
}

char* get_provision_path(const char* filename) {
    return build_horpkg_path("provision", filename);
}

int horpkg_self_dir(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }

#if defined(__APPLE__)
    uint32_t size = (uint32_t)buffer_size;
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        print_warning("Unable to determine executable path on macOS (buffer too small).");
        return -1;
    }
    char resolved[PATH_MAX];
    if (!realpath(buffer, resolved)) {
        print_warning("Failed to resolve executable path on macOS.");
        return -1;
    }
    strncpy(buffer, resolved, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", buffer, buffer_size - 1);
    if (len == -1) {
        print_warning("Failed to read /proc/self/exe to determine executable path.");
        return -1;
    }
    buffer[len] = '\0';
#else
    (void)buffer;
    (void)buffer_size;
    return -1;
#endif

    char *last_slash = strrchr(buffer, '/');
    if (!last_slash) {
        return -1;
    }
    *last_slash = '\0';
    return 0;
}

static int build_candidate_path(const char *dir, const char *filename, char *out_path, size_t out_size) {
    if (!dir || !filename || !out_path || out_size == 0) {
        return -1;
    }

    int written = snprintf(out_path, out_size, "%s/%s", dir, filename);
    if (written < 0 || (size_t)written >= out_size) {
        return -1;
    }
    if (access(out_path, R_OK) == 0) {
        canonicalize_path_inplace(out_path, out_size);
        return 0;
    }
    return -1;
}

int horpkg_find_resource(const char *filename, char *out_path, size_t out_size) {
    if (!filename || !out_path || out_size == 0) {
        return -1;
    }

    const char *env_dir = getenv("HORPKG_RESOURCES_DIR");
    if (env_dir && env_dir[0]) {
        if (build_candidate_path(env_dir, filename, out_path, out_size) == 0) {
            return 0;
        }
    }

    char exec_dir[PATH_MAX] = {0};
    if (horpkg_self_dir(exec_dir, sizeof(exec_dir)) == 0) {
        const char *relative_dirs[] = {
            "resources",
            "../resources",
            "../../resources",
            "../share/horpkg/resources",
            "../../share/horpkg/resources",
            NULL
        };
        for (int i = 0; relative_dirs[i] != NULL; i++) {
            char candidate_dir[PATH_MAX];
            int dir_written = snprintf(candidate_dir, sizeof(candidate_dir), "%s/%s", exec_dir, relative_dirs[i]);
            if (dir_written < 0 || (size_t)dir_written >= sizeof(candidate_dir)) {
                continue;
            }
            if (build_candidate_path(candidate_dir, filename, out_path, out_size) == 0) {
                return 0;
            }
        }

        if (build_candidate_path(exec_dir, filename, out_path, out_size) == 0) {
            return 0;
        }
    }

    const char *fallback_dirs[] = {
        "./resources",
        "../resources",
        "/usr/local/share/horpkg/resources",
        "/usr/share/horpkg/resources",
        NULL
    };

    for (int i = 0; fallback_dirs[i] != NULL; i++) {
        if (build_candidate_path(fallback_dirs[i], filename, out_path, out_size) == 0) {
            return 0;
        }
    }

    log_debug("Resource '%s' not found in known locations.", filename);
    return -1;
}

/**
 * @brief 尝试启动HDC服务 (system call)
 */
void hdc_start_service(void) {
    log_info("Ensuring HDC service is running...");
    system("hdc-lite start > /dev/null 2>&1");
    log_info("HDC service check complete.");
}

/**
 * @brief 检查HDC连接状态 (增加了日志)
 * @return 1 表示已连接, 0 表示未连接
 */
int hdc_is_connected(void) {
    log_debug("hdc_is_connected: Entered function");
    char line[256];
    FILE *fp = popen("hdc-lite list targets", "r");
    if (fp == NULL) {
        print_error("Failed to run 'hdc-lite'. Is it in your PATH?");
        log_debug("hdc_is_connected: popen failed, returning 0");
        return 0; 
    }
    log_debug("hdc_is_connected: popen() successful");
    // 读取第一行输出
    if (fgets(line, sizeof(line), fp) == NULL) {
        log_debug("hdc_is_connected: fgets() returned NULL (no output)");
        pclose(fp);
        log_debug("hdc_is_connected: pclose() after fgets NULL, returning 0");
        return 0; // 没有输出
    }
    log_debug("hdc_is_connected: fgets() successful");
    
    pclose(fp);
    log_debug("hdc_is_connected: pclose() successful");

    trim_whitespace(line);
    log_debug("hdc_is_connected: trim_whitespace() complete");
    // 打印修剪后的行内容
    log_debug("Trimmed line: \"%s\"", line);
    // 如果输出是 "[Empty]" 或空字符串，则未连接
    if (strcmp(line, "[Empty]") == 0 || strlen(line) == 0) {
        log_debug("hdc_is_connected: Result: Not connected (0)");
        return 0;
    }
    // 否则，假定已连接
    log_debug("hdc_is_connected: Result: Connected (1)");
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
    
    log_info("Executing connection command...");
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
    
    return uuid;
}
/**
 * @brief 检查horpkg是否已初始化 (是否已有UUID，并且UUID是否与当前设备匹配)
 * @return 1 表示已初始化且设备匹配, 0 表示未初始化或设备不匹配
 */
int is_initialized(void) {
    if (g_config.device_uuid[0] == '\0') {
        log_debug("is_initialized: device_uuid in config is empty."); // <-- [使用 log_debug]
        return 0;
    }

    char* current_uuid = hdc_get_uuid();
    if (current_uuid == NULL) {
        log_debug("is_initialized: Could not get current UUID from HDC (device disconnected?)."); // <-- [使用 log_debug]
        return 0; 
    }

    int match = (strcmp(g_config.device_uuid, current_uuid) == 0);
    
    if (match) {
        log_debug("is_initialized: Stored UUID matches current device."); // <-- [使用 log_debug]
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
