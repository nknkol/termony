#ifndef HORPKG_UTILS_H
#define HORPKG_UTILS_H

// 颜色输出
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// 版本信息
#define HORPKG_VERSION "1.0.0"

// 输出工具函数
void print_success(const char *msg);
void print_error(const char *msg);
void print_warning(const char *msg);
void print_info(const char *msg);

#endif // HORPKG_UTILS_H