#ifndef SHIM_CONFIG_H
#define SHIM_CONFIG_H
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/horpkgbin/apk"
#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"
#define CUSTOM_LIB_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib"
#define SHIM_ENV_VARS \
    X("SYSROOT", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/") \
    X("TMUX_TMPDIR", "/data/storage/el2/horpkg-base/cache")
#endif
