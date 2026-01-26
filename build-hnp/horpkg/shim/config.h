/*
 * =================================================================
 * Shim (包装器) 配置文件 (config.h) - [horpkg 专用]
 * =================================================================
 *
 * 自动化生成，用于编译 'horpkg' Shim。
 *
 */

#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H

/* * [必需] 目标命令的绝对路径
 */
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/horpkgbin/horpkg"


/* * [可选] 要 *添加* 到库路径的环境变量名
 * (例如: Linux: "LD_LIBRARY_PATH", macOS: "DYLD_LIBRARY_PATH")
 */
#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"


/*
 * [可选] 要 *添加* 在前面的库路径 (如果不需要，请设置为空字符串 "")
 */
#define CUSTOM_LIB_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib"


/*
 * [可选] 自定义环境变量列表 (X-Macro 模式)
 */
#define SHIM_ENV_VARS \
    X("TMUX_TMPDIR", "/data/storage/el2/base/cache") \
    X("SYSROOT", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/")
    /* END_SHIM_ENV_VARS */


#endif // SHIM_CONFIG_H