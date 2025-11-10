/*
 * =================================================================
 * Shim (包装器) 配置文件 (config.h) - [restool 专用]
 * =================================================================
 */

#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H

/* Restool 在 horpkgbin 中的真实路径 */
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/horpkgbin/restool"

/* 与其他工具保持一致，统一追加 horpkg 提供的系统库路径 */
#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"
#define CUSTOM_LIB_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib"

/* 执行 restool 前需要补充的环境变量 */
#define SHIM_ENV_VARS \
    X("TMUX_TMPDIR", "/data/storage/el2/horpkg-base/cache") \
    X("SYSROOT", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/")

#endif /* SHIM_CONFIG_H */
