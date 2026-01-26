#ifndef CONFIG_H
#define CONFIG_H
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/nodejs/bin/node"
#define SHIM_EXTRA_ARGS X("/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/nodejs/lib/node_modules/npm/bin/npx-cli.js")
#define SHIM_ENV_VARS \
    X("NPM_CONFIG_PREFIX", "/data/storage/el2/base/cache/node_global") \
    X("NPM_CONFIG_CACHE", "/data/storage/el2/base/cache/node_cache") \
    X("NPM_CONFIG_USERCONFIG", "/data/storage/el2/base/cache/npmrc")
#define LIB_PATH_ENV_VAR "PATH"
#define CUSTOM_LIB_PATH "/data/storage/el2/base/cache/node_global/bin:/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/nodejs/bin"
#endif
