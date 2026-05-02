#ifndef CONFIG_H
#define CONFIG_H
#define TARGET_COMMAND_PATH "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/opt/codex/bin/codex"
#define SHIM_ENV_VARS \
    X("LD_LIBRARY_PATH", "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib") \
    X("CODEX_HOME", "/data/storage/el2/base/files/.codex") \
    X("CODEX_CACHE", "/data/storage/el2/base/cache/codex") \
    X("SSL_CERT_FILE", "/etc/ssl/certs/cacert.pem") \
    X("SSL_CERT_DIR", "/etc/ssl/certs")
#endif
