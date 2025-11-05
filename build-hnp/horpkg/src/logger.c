#include "logger.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

// 全局日志级别 (由参数控制)
static log_level_t g_log_level = LOG_LEVEL_INFO;

// (私有) 级别到字符串/颜色的转换
static const char* level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "LOG";
    }
}

static const char* level_to_color(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return COLOR_CYAN;    // 青色
        case LOG_LEVEL_INFO:  return COLOR_BLUE;    // 蓝色 (用于区分 Success)
        case LOG_LEVEL_WARN:  return COLOR_YELLOW;  // 黄色
        case LOG_LEVEL_ERROR: return COLOR_RED;     // 红色
        case LOG_LEVEL_FATAL: return COLOR_MAGENTA; // 紫色
        default: return COLOR_RESET;
    }
}

void logger_init(log_level_t level) {
    g_log_level = level;
    log_debug("Logger initialized with level %d", level);
}

void logger_set_level(log_level_t level) {
    g_log_level = level;
}

void logger_log(log_level_t level, const char *file, int line, const char *fmt, ...) {
    // 1. 检查级别 (参数控制)
    if (level < g_log_level) {
        return;
    }
    
    // 2. 确定输出流 (ERROR/FATAL 输出到 stderr)
    FILE *stream = (level >= LOG_LEVEL_ERROR) ? stderr : stdout;
    
    // 3. 格式化时间
    char time_str[20];
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", lt);

    // 4. 打印日志头
    fprintf(stream, "%s[%s] [%-5s]%s ",
            level_to_color(level),
            time_str,
            level_to_string(level),
            COLOR_RESET
    );

    // 5. 打印消息
    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    // 6. (可选) 打印 ERROR/FATAL 的来源
    if (file != NULL && level >= LOG_LEVEL_ERROR) {
        fprintf(stream, " %s(%s:%d)%s", COLOR_RED, file, line, COLOR_RESET);
    }
    
    fprintf(stream, "\n");
    fflush(stream); // 确保立即输出

    // FATAL 级别终止程序
    if (level == LOG_LEVEL_FATAL) {
        exit(EXIT_FAILURE);
    }
}