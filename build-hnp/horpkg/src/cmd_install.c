#include "commands.h"
#include "utils.h"
#include "config.h"
#include "logger.h"
#include "install.h"
#include "hap_parser.h"
#include "signing.h"
#include "auth.h"
#include "acl_whitelist.h"
#include "hnp_utils.h"

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
    // Runtime package assumed to have standard permissions (or none requiring ACLs for basic install)
    if (signing_ensure_provision_for_bundle(&user, bundle_name, &cert, NULL, 0, provision_path, sizeof(provision_path)) != 0) {
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
    
    // 3. 提取并验证权限
    char **perms = NULL;
    int perm_count = 0;
    
    // 用于 Profile 申请的过滤后的权限列表
    char **perms_to_request = NULL;
    int perms_to_request_count = 0;

    if (hap_parser_get_permissions(unsigned_hap_path, &perms, &perm_count) != 0) {
        log_warn("Failed to parse permissions from HAP. Assuming no extra permissions needed.");
    }
    
    if (perm_count > 0) {
        log_info("Checking %d requested permissions against rules...", perm_count);
        
        perms_to_request = malloc(perm_count * sizeof(char*));
        
        for (int i = 0; i < perm_count; i++) {
            const char *p = perms[i];
            perm_check_result_t type = check_permission_type(p);
            
            if (type == PERM_TYPE_OPEN) {
                // 开放权限，无需处理，直接放行
                log_debug("Permission '%s' is OPEN (User/System Grant). Skipped for Profile.", p);
            } else if (type == PERM_TYPE_ACL_AUTO) {
                // 受限权限 (自动签名支持)，加入申请列表
                log_info("Permission '%s' requires ACL (Auto-Sign Supported). Adding to Profile.", p);
                perms_to_request[perms_to_request_count++] = p; // 指针复用
            } else {
                // 受限权限 (需手动申请) -> 报错
                print_error_fmt("Permission '%s' is RESTRICTED and NOT supported for auto-signing.", p);
                print_error("Please request a manual profile with this permission and import it.");
                
                // Cleanup
                free(perms_to_request);
                for(int k=0; k<perm_count; k++) free(perms[k]);
                free(perms);
                return -1;
            }
        }
    }

    // 重试机制：最多尝试 2 次 (第一次失败后，删除 profile 重试)
    int max_retries = 1;
    int attempt = 0;
    int success = 0;
    
    while (attempt <= max_retries) {
        
        // 4. 确保 Provision Profile 存在 (本地或下载)
        //    (每次循环都调用，因为如果删除了文件，这里会负责重新下载)
        //    注意: 这里传递 perms_to_request (过滤后的列表)
        char profile_path[512];
        if (signing_ensure_provision_for_bundle(&user, bundle_name, &cert, perms_to_request, perms_to_request_count, profile_path, sizeof(profile_path)) != 0) {
            print_error_fmt("Failed to get provision profile for bundle: %s", bundle_name);
            break; // Break retry loop on provision failure
        }
        
        // 5. 获取其他签名文件路径
        char *keystore_path = get_signature_path("horpkg.p12");
        char *cert_path = get_signature_path("horpkg.cer"); // hapsigntool 需要 appCertFile
        
        if (access(keystore_path, F_OK) != 0 || access(cert_path, F_OK) != 0) {
            log_error("Missing 'horpkg.p12' or 'horpkg.cer'.");
            print_prompt("Please run 'horpkg init' to generate credentials.");
            free(keystore_path);
            free(cert_path);
            break;
        }

        // 6. 构建 hapsigntool 命令
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

        if (attempt == 0) {
            log_info("Executing hapsigntool...");
        } else {
            log_info("Retrying hapsigntool (Attempt %d)...", attempt + 1);
        }
        log_debug("Sign CMD: %s", cmd);

        // 7. 执行签名
        int status = system(cmd);
        
        free(keystore_path);
        free(cert_path);
        
        if (status == 0) {
            print_success("HAP file signed successfully.");
            success = 1;
            break;
        }
        
        // --- 失败处理逻辑 ---
        
        if (attempt < max_retries) {
            print_warning_fmt("Signing failed (Exit code %d). The local profile might be invalid.", status);
            print_info("Attempting auto-recovery: Deleting local profile and retrying...");
            
            if (access(profile_path, F_OK) == 0) {
                if (unlink(profile_path) == 0) {
                    log_info("Deleted invalid profile: %s", profile_path);
                } else {
                    log_warn("Failed to delete profile: %s", profile_path);
                }
            }
            // 增加计数，进入下一次循环
            attempt++;
            continue;
        } else {
            // 重试也失败了
            print_error_fmt("hapsigntool failed with exit code %d after retries.", status);
            print_error_fmt("Failed to sign: %s", unsigned_hap_path);
            break;
        }
    }
    
    // Cleanup perms
    if (perms_to_request) free(perms_to_request); // 只是指针数组，里面的字符串在 perms 里
    for(int i=0; i<perm_count; i++) free(perms[i]);
    free(perms);

    return success ? 0 : -1;
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

    // --- [新增] HNP 规范化处理 ---
    char normalized_hap_path[512] = {0};
    if (hnp_utils_check_tools() == 0) {
        snprintf(normalized_hap_path, sizeof(normalized_hap_path), "%s/normalized_%d.hap", tmp_dir, getpid());
        log_info("Checking for HNP packages to normalize...");
        
        int repack_rc = hnp_repack_hap(hap_path, normalized_hap_path);
        if (repack_rc == 0) {
            log_info("HNP normalization successful. Using optimized HAP.");
            hap_path = normalized_hap_path; // 切换指向
        } else {
            // 失败可能是因为没有HNP包（我们在hnp_repack_hap里处理了），或者出错
            // 如果出错是否终止？目前 hnp_repack_hap 如果没找到HNP会打印日志并返回-1 (在我的实现里)
            // 实际上，如果仅仅是没有HNP，应该返回特定值或静默。
            // 让我们假定如果失败且没生成文件，就继续用原文件。
            if (access(normalized_hap_path, F_OK) != 0) {
                log_debug("Skipping HNP normalization (no changes or tools failed).");
            }
        }
    } else {
        log_debug("Skipping HNP normalization (hnpcli/restool not found).");
    }
    // ---------------------------

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
        // 清理临时文件
        if (normalized_hap_path[0] && access(normalized_hap_path, F_OK) == 0) unlink(normalized_hap_path);
        return -1;
    }
    logger_log(LOG_LEVEL_INFO, NULL, 0, "Signed HAP saved at: %s", signed_hap_path);

    // 4. 安装 HAP
    log_info("Step 3/3: Installing HAP via HDC...");
    int install_result = hdc_install_hap(signed_hap_path, bundle_name, local_install_prompt);
    
    // 清理临时文件
    unlink(signed_hap_path);
    if (normalized_hap_path[0] && access(normalized_hap_path, F_OK) == 0) unlink(normalized_hap_path);

    if (install_result != 0) {
        log_error("HDC installation failed or was cancelled.");
        return -1;
    }

    print_success_fmt("Successfully installed local HAP: %s", hap_path);
    return 0;
}
