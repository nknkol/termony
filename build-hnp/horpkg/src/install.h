#ifndef HORPKG_INSTALL_H
#define HORPKG_INSTALL_H

#include "hdc.h" // for hdc_install_action_t

/**
 * @brief (内部) 从本地 HAP/HSP 文件安装。
 * 流程: 解析 -> 签名 -> 安装。
 * @param hap_path .hap 文件的路径。
 * @param provided_pin 针对受保护包的 PIN（可为 NULL）。
 * @return 0 成功, -1 失败。
 */
int install_local_hap(const char *hap_path, const char *provided_pin);

/**
 * @brief (内部) 使用 horpkg 凭证对 HAP 文件进行签名。
 *
 * @param unsigned_hap_path 原始 .hap 文件的路径。
 * @param bundle_name 包名 (用于查找或创建 provision 配置文件)。
 * @param signed_hap_out_path [输出] 签名后的 .hap 文件的保存路径。
 * @return 0 成功, -1 失败。
 */
int sign_hap(const char *unsigned_hap_path, const char *bundle_name, const char *signed_hap_out_path);

/**
 * @brief (内部) 当本地安装检测到包已存在时，用于 hdc_install_hap 的回调。
 */
hdc_install_action_t local_install_prompt(const char *bundleName, const char *installed_version, const char *new_version);


#endif // HORPKG_INSTALL_H
