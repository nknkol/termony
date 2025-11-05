/*
 * =================================================================
 * Shim (包装器) 配置文件 (config.h)
 * =================================================================
 */

#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H

/* * [必需] 目标命令的绝对路径
 * (安装在 horpkgbin 目录中)
 */
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/horpkgbin/hapsigntool"


/* * [可选] 要 *添加* 到库路径的环境变量名
 */
#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"


/*
 * [可选] 要 *添加* 在前面的库路径
 * (与 horpkg 共享相同的 lib 路径)
 */
#define CUSTOM_LIB_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib"


/*
 * [可选] 自定义环境变量列表 (X-Macro 模式)
 * (与 horpkg 共享相同的环境)
 */
#define SHIM_ENV_VARS \
    X("TMUX_TMPDIR", "/data/storage/el2/horpkg-base/cache") \
    X("SYSROOT", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/")
    /* END_SHIM_ENV_VARS */


#endif // SHIM_CONFIG_H