#ifndef HORPKG_HAP_PARSER_H
#define HORPKG_HAP_PARSER_H

#include <stddef.h> // for size_t

/**
 * @brief 使用 libzip 和 yyjson 从 HAP 文件中提取 bundleName。
 *
 * 它会依次检查 "module.json", "pack.info", "config.json", "module.json5"。
 * 如果找到多个冲突的 bundleName，将返回错误。
 *
 * @param hap_path 指向 .hap 文件的路径。
 * @param bundle_name_out [输出] 用于存储找到的 bundleName 的缓冲区。
 * @param bundle_name_size 缓冲区的最大大小。
 * @return 0 成功找到唯一的 bundleName，-1 失败 (文件未找到, zip错误, json解析错误, 未找到, 或冲突)。
 */
int hap_parser_get_bundle_name(const char *hap_path, char *bundle_name_out, size_t bundle_name_size);

#endif // HORPKG_HAP_PARSER_H