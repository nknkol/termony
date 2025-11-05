#ifndef HORPKG_LOGGER_H
#define HORPKG_LOGGER_H

#include <stdio.h>
#include <stdarg.h>

// 1. 定义日志级别
typedef enum {
    LOG_LEVEL_DEBUG = 0, // 调试信息
    LOG_LEVEL_INFO,      // 普通信息
    LOG_LEVEL_WARN,      // 警告
    LOG_LEVEL_ERROR,     // 错误
    LOG_LEVEL_FATAL,     // 致命错误 (将导致程序退出)
    LOG_LEVEL_NONE       // (用于完全关闭)
} log_level_t;

// (颜色定义)
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/**
 * @brief 初始化日志记录器。
 * (这是您要求的“参数开启”)
 * @param level 低于此级别的日志将被忽略。
 */
void logger_init(log_level_t level);

/**
 * @brief 设置新的日志级别。
 */
void logger_set_level(log_level_t level);

/**
 * @brief 核心日志函数。
 * @param level 日志级别。
 * @param file 源代码文件名 (__FILE__)。
 * @param line 源代码行号 (__LINE__)。
 * @param fmt 格式化字符串。
 * @param ... 可变参数。
 */
void logger_log(log_level_t level, const char *file, int line, const char *fmt, ...);

// 3. 封装宏 (简化调用)
// (这些是供新代码调用的宏)

// DEBUG 和 INFO 默认不包含文件/行号
#define log_debug(fmt, ...) logger_log(LOG_LEVEL_DEBUG, NULL, 0, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  logger_log(LOG_LEVEL_INFO,  NULL, 0, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  logger_log(LOG_LEVEL_WARN,  NULL, 0, fmt, ##__VA_ARGS__)
// ERROR 和 FATAL 自动附加文件/行号
#define log_error(fmt, ...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_fatal(fmt, ...) logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif // HORPKG_LOGGER_H