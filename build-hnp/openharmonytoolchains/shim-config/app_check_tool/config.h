/* Auto-generated shim config for app_check_tool */
#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H

#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/jdk/bin/java"

#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"
#define CUSTOM_LIB_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib"

#define SHIM_EXTRA_ARGS \
    X("-jar") \
    X("/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/jdk/lib/app_check_tool.jar")

#define SHIM_ENV_VARS \
    X("TMUX_TMPDIR", "/data/storage/el2/base/cache") \
    X("TMPDIR", "/data/storage/el2/base/cache") \
    X("TMP", "/data/storage/el2/base/cache") \
    X("TEMP", "/data/storage/el2/base/cache") \
    X("JAVA_TOOL_OPTIONS", "-Djava.io.tmpdir=/data/storage/el2/base/cache") \
    X("SYSROOT", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/")

#endif /* SHIM_CONFIG_H */
