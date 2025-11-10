#ifndef HORPKG_SHIM_INSTALLER_H
#define HORPKG_SHIM_INSTALLER_H

/**
 * @brief 根据包内 HNP (zip) 的 shim/manifest.json 编译并安装 shim。
 *
 * @param package_name 当前包名，仅用于日志/缺省 manifest 字段。
 * @param hnp_path     HNP 文件路径 (zip 格式)。
 * @return 0 表示成功或 manifest 不存在 (视为无需处理)，非 0 表示失败。
 */
int shim_install_from_hnp(const char *package_name, const char *hnp_path);

#endif // HORPKG_SHIM_INSTALLER_H
