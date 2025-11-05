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
void print_prompt(const char *msg);
// 格式化输出工具函数
void print_error_fmt(const char *fmt, ...);
void print_warning_fmt(const char *fmt, ...);
void print_info_fmt(const char *fmt, ...);
void print_success_fmt(const char *fmt, ...); // <-- [新增这一行]
// 文件和路径工具
char* get_config_path(const char* filename);
int create_dir_if_not_exists(const char *path);
// HDC 和初始化工具
void hdc_start_service(void);
int hdc_is_connected(void);
int hdc_connect_port(const char *port);
char* hdc_get_uuid(void);
int store_uuid(const char* uuid);
int is_initialized(void);

#endif