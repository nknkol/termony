#ifndef HORPKG_HDC_H
#define HORPKG_HDC_H

#include <stddef.h> // for size_t

/**
 * @brief 当尝试安装已存在的包时，回调函数返回的操作。
 */
typedef enum {
    HDC_INSTALL_CANCEL,     // 取消安装
    HDC_INSTALL_OVERWRITE   // 覆盖安装
} hdc_install_action_t;

/**
 * @brief 安装回调函数指针类型。
 *
 * @param bundleName 包名。
 * @param installed_version 已安装的版本 (如果未知则为 "unknown")。
 * @param new_version 准备安装的新版本 (如果未知则为 "unknown")。
 * @return 用户选择的操作 (HDC_INSTALL_CANCEL 或 HDC_INSTALL_OVERWRITE)。
 */
typedef hdc_install_action_t (*hdc_install_callback_t)(const char *bundleName, const char *installed_version, const char *new_version);

/**
 * @brief 检查指定的 bundleName 是否已安装在设备上。
 *
 * @param bundleName 要检查的包名 (例如 "com.example.app")。
 * @param version_name_out [输出] 用于存储版本名称的缓冲区。
 * @param version_name_size 缓冲区的最大大小。
 * @return 1 如果已安装 (version_name_out 被填充)，0 如果未安装，-1 如果出错。
 */
int hdc_check_if_installed(const char *bundleName, char *version_name_out, size_t version_name_size);

/**
 * @brief 将 HAP/HSP 文件安装到设备。
 *
 * @param file_path 要安装的 .hap 或 .hsp 文件的路径。
 * @param bundleName 该文件的包名 (用于检查是否已安装)。
 * @param callback 一个函数指针，当包已存在时调用该函数询问用户操作。
 * @return 0 成功安装，-1 安装失败或被用户取消。
 */
int hdc_install_hap(const char *file_path, const char *bundleName, hdc_install_callback_t callback);

#endif // HORPKG_HDC_H