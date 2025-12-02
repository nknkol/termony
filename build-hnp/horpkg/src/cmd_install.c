#include "commands.h"
#include "utils.h"
#include "config.h"
#include "logger.h"
#include "install.h"
#include "hap_parser.h"
#include "signing.h"
#include "auth.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef HORPKG_RUNTIME_PIN
#define HORPKG_RUNTIME_PIN "314159"
#endif

static int uninstall_runtime_bundle_if_installed(const char *bundle_name) {
    if (!bundle_name || !bundle_name[0]) {
        return -1;
    }
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "hdc-lite uninstall %s > /dev/null 2>&1", bundle_name);
    log_info("Ensuring previous runtime bundle is removed before install...");
    int status = system(cmd);
    if (status != 0) {
        log_warn("Uninstall command returned non-zero. The bundle may not have been installed.");
        return -1;
    }
    log_info("Runtime bundle uninstall step completed.");
    return 0;
}

// [!]
// [!] 这是 `install` 命令的主入口
// [!]

static int ensure_runtime_signing_ready(const char *bundle_name) {
    user_info_t user = {0};
    config_apply_auth_to_user(&user);
    if (user.access_token[0] == '\0' || user.jwt_token[0] == '\0') {
        log_error("Runtime install requires valid authentication. Please run 'horpkg init'.");
        return -1;
    }

    cert_info_t cert = {0};
    if (g_config.cert_id[0] == '\0') {
        log_error("Missing certificate ID for runtime. Please run 'horpkg init'.");
        return -1;
    }
    strncpy(cert.id, g_config.cert_id, sizeof(cert.id) - 1);

    char *keystore_path = get_signature_path("horpkg.p12");
    char *cert_path = get_signature_path("horpkg.cer");
    int missing = (access(keystore_path, F_OK) != 0 || access(cert_path, F_OK) != 0);
    free(keystore_path);
    free(cert_path);
    if (missing) {
        log_error("Missing signing materials (horpkg.p12 / horpkg.cer). Please run 'horpkg init'.");
        return -1;
    }

    char provision_path[512] = {0};
    if (signing_ensure_provision_for_bundle(&user, bundle_name, &cert, provision_path, sizeof(provision_path)) != 0) {
        log_error("Failed to ensure provision profile for runtime package.");
        return -1;
    }

    log_info("Runtime signing profile ready: %s", provision_path);
    return 0;
}

int cmd_install(int argc, char *argv[]) {
    const char *provided_pin = NULL;
    const char *package_arg = NULL;

    if (argc >= 3 && strcmp(argv[0], "pin") == 0) {
        provided_pin = argv[1];
        package_arg = argv[2];
    } else if (argc >= 1) {
        package_arg = argv[0];
    } else {
        log_error("Local HAP/HSP file path required.");
        printf("Usage: horpkg install [pin <PIN>] ./path/to/app.hap\n");
        return 1;
    }

    // --- 检查初始化 (所有安装都需要) ---
    if (!is_initialized()) {
        log_error("Horpkg not initialized.");
        print_prompt("Please run 'horpkg init' first to register your device.");
        return 1;
    }

    if (auth_ensure_valid_session(NULL) != 0) {
        return 1;
    }

    // 仅支持本地安装，参数必须指向本地文件
    if (strncmp(package_arg, "./", 2) != 0 &&
        strncmp(package_arg, "../", 3) != 0 &&
        strncmp(package_arg, "/", 1) != 0 &&
        !(strlen(package_arg) > 4 && (!strcmp(package_arg + strlen(package_arg) - 4, ".hap") ||
                                      !strcmp(package_arg + strlen(package_arg) - 4, ".hsp")))) {
        print_error("Only local HAP/HSP files are supported. Please provide a file path.");
        return 1;
    }

    print_info_fmt("Starting local file installation for: %s", package_arg);
    return install_local_hap(package_arg, provided_pin);
}


// [!]
// [!] 本地安装回调
// [!]
hdc_install_action_t local_install_prompt(const char *bundleName, const char *installed_version, const char *new_version) {
    log_warn("Package %s (Version %s) is already installed.", bundleName, installed_version);
    
    char input[10];
    while (1) {
        printf("%s?%s Do you want to (O)verwrite or (C)ancel? [o/C]: ", COLOR_YELLOW, COLOR_RESET);
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            return HDC_INSTALL_CANCEL; // EOF 或读取错误
        }
        
        // 移除换行符
        input[strcspn(input, "\n")] = 0;

        if (input[0] == 'o' || input[0] == 'O') {
            return HDC_INSTALL_OVERWRITE;
        }
        if (input[0] == 'c' || input[0] == 'C' || input[0] == '\0') {
            return HDC_INSTALL_CANCEL;
        }
        // 其他输入，循环
    }
}


// [!]
// [!] 本地 HAP 签名
// [!]
int sign_hap(const char *unsigned_hap_path, const char *bundle_name, const char *signed_hap_out_path) {
    
    log_info("Loading signing credentials from config...");
    
    // (config_load() 已在 main.c 中调用)
    
    // 1. 加载认证 (用于 API 调用)
    user_info_t user = {0};
    config_apply_auth_to_user(&user);
    if (user.access_token[0] == '\0') {
        log_error("Authentication token not found. Please run 'horpkg init'.");
        return -1;
    }

    // 2. 加载证书 (用于 API 调用)
    cert_info_t cert = {0};
    if (g_config.cert_id[0] == '\0') {
        log_error("Certificate ID not found. Please run 'horpkg init'.");
        return -1;
    }
    strncpy(cert.id, g_config.cert_id, sizeof(cert.id) - 1);
    
    // 3. 确保 Provision Profile 存在 (本地或下载)
    char profile_path[512];
    if (signing_ensure_provision_for_bundle(&user, bundle_name, &cert, profile_path, sizeof(profile_path)) != 0) {
        print_error_fmt("Failed to get provision profile for bundle: %s", bundle_name);
        return -1;
    }
    
    // 4. 获取其他签名文件路径
    char *keystore_path = get_signature_path("horpkg.p12");
    char *cert_path = get_signature_path("horpkg.cer"); // hapsigntool 需要 appCertFile
    
    if (access(keystore_path, F_OK) != 0 || access(cert_path, F_OK) != 0) {
        log_error("Missing 'horpkg.p12' or 'horpkg.cer'.");
        print_prompt("Please run 'horpkg init' to generate credentials.");
        free(keystore_path);
        free(cert_path);
        return -1;
    }

    // 5. 构建 hapsigntool 命令
    char cmd[4096];
    const char *lib_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib";

    snprintf(cmd, sizeof(cmd),
        "LD_LIBRARY_PATH=\"%s\" "
        "hapsigntool sign-app "
        "-mode localSign "
        "-keyAlias \"horpkg\" "  // (使用 init 中设置的别名)
        "-signAlg \"SHA256withECDSA\" "
        "-appCertFile \"%s\" "  // (horpkg.cer)
        "-profileFile \"%s\" "  // (bundleName.p7b)
        "-inFile \"%s\" "       // (输入 HAP)
        "-compatibleVersion 8 " // (根据您的示例)
        "-keystoreFile \"%s\" " // (horpkg.p12)
        "-outFile \"%s\" "      // (输出 HAP)
        "-keyPwd \"horpkg\" "   // (使用 init 中设置的密码)
        "-keystorePwd \"horpkg\" " // (使用 init 中设置的密码)
        "-signCode 1 "
        "2>&1",
        lib_path,
        cert_path,
        profile_path,
        unsigned_hap_path,
        keystore_path,
        signed_hap_out_path
    );

    log_info("Executing hapsigntool...");
    log_debug("Sign CMD: %s", cmd);

    // 6. 执行签名
    int status = system(cmd);
    
    free(keystore_path);
    free(cert_path);
    
    if (status != 0) {
        print_error_fmt("hapsigntool failed with exit code %d.", status);
        print_error_fmt("Failed to sign: %s", unsigned_hap_path);
        return -1;
    }
    
    print_success("HAP file signed successfully.");
    return 0;
}


// [!]
// [!] 本地安装流程
// [!]
int install_local_hap(const char *hap_path, const char *provided_pin) {
    
    // 1. 检查文件是否存在
    if (access(hap_path, F_OK) != 0) {
        print_error_fmt("Local file not found: %s", hap_path);
        return -1;
    }

    // 2. 解析 HAP 获取 bundleName
    log_info("Step 1/3: Parsing HAP file...");
    char bundle_name[256];
    if (hap_parser_get_bundle_name(hap_path, bundle_name, sizeof(bundle_name)) != 0) {
        print_error_fmt("Failed to get bundleName from %s.", hap_path);
        return -1;
    }
    print_success_fmt("Found bundleName: %s", bundle_name);

    // 保留包名: org.horpkg.runtime 仅检查签名配置/HDC（HDC 在 main 已校验）
    if (strcmp(bundle_name, "org.horpkg.runtime") == 0) {
        const char *PIN_CODE = HORPKG_RUNTIME_PIN; // 编译时固定 PIN (可由宏覆盖)
        if (!provided_pin) {
            print_error("Runtime package install requires PIN. Usage: horpkg install pin <PIN> <hap_path>");
            return -1;
        }
        if (strcmp(provided_pin, PIN_CODE) != 0) {
            log_error("Invalid PIN for runtime package.");
            return -1;
        }
        if (ensure_runtime_signing_ready(bundle_name) != 0) {
            return -1;
        }
        if (uninstall_runtime_bundle_if_installed(bundle_name) != 0) {
            log_error("Failed to uninstall existing runtime bundle. Aborting install.");
            return -1;
        }
    }

    // 3. 对 HAP 进行签名
    log_info("Step 2/3: Signing HAP...");

    char *tmp_dir = get_config_path("tmp");
    if (!tmp_dir) {
        log_error("Failed to get temporary directory path.");
        return -1;
    }
    create_dir_if_not_exists(tmp_dir); // 确保 tmp 目录存在

    char signed_hap_path[512];
    if (hap_path[0] == '/' || strstr(hap_path, ":/") != NULL) {
        // 绝对路径或包含驱动器路径 -> 在同目录生成 .signed.hap
        const char *last_sep = strrchr(hap_path, '/');
        if (!last_sep) {
            snprintf(signed_hap_path, sizeof(signed_hap_path), "%s.signed.hap", hap_path);
        } else {
            size_t prefix_len = last_sep - hap_path;
            char dir_prefix[512];
            snprintf(dir_prefix, sizeof(dir_prefix), "%.*s", (int)prefix_len, hap_path);
            const char *filename = last_sep + 1;
            char base_name[256];
            snprintf(base_name, sizeof(base_name), "%s", filename);
            char *dot = strrchr(base_name, '.');
            if (dot) *dot = '\0';
            snprintf(signed_hap_path, sizeof(signed_hap_path), "%s/%s.signed.hap", dir_prefix, base_name);
        }
    } else {
        // 相对路径 -> 使用 tmp 目录
        const char *filename = strrchr(hap_path, '/');
        filename = filename ? filename + 1 : hap_path;
        char base_name[256];
        snprintf(base_name, sizeof(base_name), "%s", filename);
        char *dot = strrchr(base_name, '.');
        if (dot) *dot = '\0';
        snprintf(signed_hap_path, sizeof(signed_hap_path), "%s/%s.signed.hap", tmp_dir, base_name);
    }
    free(tmp_dir); // 释放路径

    logger_log(LOG_LEVEL_INFO, NULL, 0, "Signing output will be saved to: %s", signed_hap_path);

    if (sign_hap(hap_path, bundle_name, signed_hap_path) != 0) {
        log_error("Failed to sign HAP file.");
        unlink(signed_hap_path);
        return -1;
    }
    logger_log(LOG_LEVEL_INFO, NULL, 0, "Signed HAP saved at: %s", signed_hap_path);

    // 4. 安装 HAP
    log_info("Step 3/3: Installing HAP via HDC...");
    int install_result = hdc_install_hap(signed_hap_path, bundle_name, local_install_prompt);
    
    if (install_result != 0) {
        log_error("HDC installation failed or was cancelled.");
        return -1;
    }

    print_success_fmt("Successfully installed local HAP: %s", hap_path);
    return 0;
}
