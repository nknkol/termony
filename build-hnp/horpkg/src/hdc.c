#define _DEFAULT_SOURCE // [!] 为 popen/pclose 添加
#include <stdbool.h>
#include "hdc.h"
#include "logger.h"
#include "utils.h" // [!] 添加
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

// 内部助手函数：修剪字符串前后的空白符
static void trim_whitespace_inplace(char *str) {
    if (!str) return;

    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }

    // 将非空白部分移到开头
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // 移除尾部空白
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
}

static bool line_has_error_token(const char *line) {
    if (!line) return false;
    char lower[512];
    size_t len = strlen(line);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;
    for (size_t i = 0; i < len; i++) {
        lower[i] = (char)tolower((unsigned char)line[i]);
    }
    lower[len] = '\0';
    return strstr(lower, "error") != NULL;
}

/**
 * @brief 从 'bm dump' 的输出中解析 versionName。
 * 示例输入: "    versionName: 1.0.0"
 */
static int parse_version_from_output(char *output, char *version_out, size_t version_size) {
    char *line = strtok(output, "\n");
    while (line != NULL) {
        char* key = strstr(line, "versionName:");
        if (key) {
            char* value = key + strlen("versionName:");
            trim_whitespace_inplace(value);
            strncpy(version_out, value, version_size - 1);
            version_out[version_size - 1] = '\0';
            return 1; // 找到
        }
        line = strtok(NULL, "\n");
    }
    return 0; // 未找到
}

int hdc_check_if_installed(const char *bundleName, char *version_name_out, size_t version_name_size) {
    char cmd[512];
    // 仅查询 versionName 以减少输出
    snprintf(cmd, sizeof(cmd), "hdc-lite shell bm dump -n %s | grep 'versionName:'", bundleName);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        print_error_fmt("Failed to execute 'hdc-lite shell bm dump' for %s", bundleName);
        return -1;
    }

    char buffer[1024] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    pclose(fp);

    if (bytes_read == 0) {
        log_debug("Package %s not found (bm dump returned no output).", bundleName);
        return 0; // 未安装
    }

    // 检查是否有失败提示 (尽管 grep 应该会过滤掉)
    if (strstr(buffer, "fail") || strstr(buffer, "not found")) {
        log_debug("Package %s not found (bm dump returned error).", bundleName);
        return 0; // 未安装
    }

    // 解析版本
    if (parse_version_from_output(buffer, version_name_out, version_name_size)) {
        log_debug("Found installed package %s, version %s", bundleName, version_name_out);
        return 1; // 已安装
    }

    log_warn("Command output for %s was ambiguous, assuming not installed.", bundleName);
    return 0;
}

int hdc_install_hap(const char *file_path, const char *bundleName, hdc_install_callback_t callback) {
    char installed_version[128] = "unknown";
    int install_status = hdc_check_if_installed(bundleName, installed_version, sizeof(installed_version));

    char cmd[1024];
    int hdc_ret;
    hdc_install_action_t action = HDC_INSTALL_OVERWRITE; // 默认

    if (install_status == 1) {
        // 包已安装，调用回调
        if (callback) {
            // TODO: 从 HAP 中解析新版本号以提供给回调
            action = callback(bundleName, installed_version, "new");
        }
        
        if (action == HDC_INSTALL_CANCEL) {
            log_info("Installation cancelled by user.");
            return -1;
        }
        
        // 覆盖安装
        log_info("Attempting to overwrite existing application...");
        snprintf(cmd, sizeof(cmd), "hdc-lite install -r \"%s\"", file_path);
        
    } else if (install_status == 0) {
        // 首次安装
        log_info("Attempting new application install...");
        snprintf(cmd, sizeof(cmd), "hdc-lite install \"%s\"", file_path);
        
    } else {
        // 检查时出错
        log_error("Failed to check installation status before install.");
        return -1;
    }

    print_info_fmt("Executing HDC command: %s", cmd);
    fflush(stdout);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        print_error("Failed to spawn hdc-lite process.");
        return -1;
    }

    char output_line[512];
    int saw_error = 0;
    while (fgets(output_line, sizeof(output_line), pipe)) {
        fputs(output_line, stdout);
        if (line_has_error_token(output_line)) {
            saw_error = 1;
        }
    }

    int status = pclose(pipe);
    if (status == -1) {
        print_error("Failed to retrieve hdc-lite exit status.");
        return -1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        print_error_fmt("HDC install command failed with exit code %d.", WEXITSTATUS(status));
        return -1;
    }

    if (saw_error) {
        print_error("HDC reported error messages during installation.");
        return -1;
    }

    print_success("HDC install command executed successfully.");
    return 0;
}
