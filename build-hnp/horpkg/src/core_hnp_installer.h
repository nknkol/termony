#ifndef HORPKG_CORE_HNP_INSTALLER_H
#define HORPKG_CORE_HNP_INSTALLER_H

/**
 * @brief 将下载好的 HNP 注入本地 org.horpkg.core.hap，重新签名并安装。
 *
 * @param package_name    包名（例如 "tree"），用于状态记录。
 * @param package_version 包版本（例如 "2.2.1"），为空则使用 "0.0.0"。
 * @param hnp_path        下载到本地的 HNP 路径。
 * @param source_url      HNP 原始下载地址（用于缺失时重拉）。
 * @param source_sha256   HNP 校验哈希，可为空。
 * @return 0 成功，非 0 失败。
 */
int core_install_hnp(const char *package_name, const char *package_version, const char *hnp_path,
                     const char *source_url, const char *source_sha256);

#endif // HORPKG_CORE_HNP_INSTALLER_H
