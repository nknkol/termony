/*
 * =================================================================
 * Shim (包装器) 配置文件 (config.h)  —— hnpcli
 * =================================================================
 */

#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H

/* 目标命令的绝对路径 (安装在 horpkgbin 目录中) */
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/horpkgbin/hnpcli"

/* 要 *添加* 到库路径的环境变量名 */
#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"

/* horpkg 运行时共享的 lib 路径 */
#define CUSTOM_LIB_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib"

/* 与其它工具保持一致的基础环境变量 */
#define SHIM_ENV_VARS \
    X("TMUX_TMPDIR", "/data/storage/el2/horpkg-base/cache") \
    X("SYSROOT", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/")
    /* END_SHIM_ENV_VARS */

#endif /* SHIM_CONFIG_H */
