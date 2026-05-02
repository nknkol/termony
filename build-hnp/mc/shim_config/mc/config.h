#ifndef CONFIG_H
#define CONFIG_H
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/mc/bin/mc"
#define SHIM_ENV_VARS \
    X("LD_LIBRARY_PATH", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib:/data/service/hnp/horpkg-base.org/horpkg-base_1.0/usr/lib") \
    X("MC_DATADIR", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/mc/share/mc") \
    X("MC_HOME", "/data/storage/el2/base/files/.mc") \
    X("TERM", "xterm-256color")
#endif
