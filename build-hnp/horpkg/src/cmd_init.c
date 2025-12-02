#include "commands.h"
#include "utils.h"
#include "config.h"
#include "auth.h"
#include "signing.h"
#include "logger.h"
#include "hap_parser.h"
#include "hdc.h"
#include "install.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h> // for free()
#include <string.h> // for memset
#include <unistd.h> // for access
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// [!] 注意: `cmd_install` 已移至 `cmd_install.c`
// [!] 移除了 `download_file` 原型 (已在 `download.h` 中)

/**
 * 核心初始化函数 (包含高级容错逻辑)
 */
int cmd_init(int argc, char *argv[]) {
    printf("\n%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Horpkg Initialization                 ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n\n", COLOR_CYAN, COLOR_RESET);

    // ===== 阶段1: 基础配置 =====
    // (config_load() 已经在 main.c 中调用)
    log_info("Configuration loaded.");
    
    // ===== 阶段2: 设备 UUID =====
    // (此处的 is_initialized() 和 store_uuid() 已被重构，使用 g_config)
    if (!is_initialized()) {
        log_info("Getting device UUID via HDC...");
        char* uuid = hdc_get_uuid();
        if (uuid) {
            if (store_uuid(uuid) == 0) { // (现在会保存到 config.json)
                log_info("Device UUID retrieved and saved:");
                printf("    UUID: %s\n\n", uuid);
            } else {
                log_error("Failed to store device UUID.");
                free(uuid);
                return 1;
            }
            free(uuid);
        } else {
            log_error("Failed to get device UUID via HDC.");
            return 1;
        }
    } else {
        log_info("Device UUID already registered.\n");
    }
    
    // ===== 阶段3: 华为账号认证 =====
    log_info("Step 1: Huawei Developer Account Authentication");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    user_info_t user = {0};
    
    int need_re_auth = 1;     // 默认需要重新认证
    // [!] `device_list_ok` 等变量移至 `signing_ensure_provision_for_bundle`
    
    // (重构: 从 g_config 加载 token)
    if (g_config.auth.jwt_token[0] != '\0' && g_config.auth.access_token[0] != '\0') {
        // (将 g_config 应用到本地 user 变量)
        config_apply_auth_to_user(&user);
        log_info("Verifying saved authentication against AGC device list...");

        // (使用 API 4.1 获取设备列表作为验证)
        char **device_ids = NULL;
        char **device_names = NULL;
        int device_count = 0;
        if (signing_get_device_list(&user, &device_ids, &device_names, &device_count) == 0) {
            log_info("Using existing authentication");
            printf("    User: %s (%s)\n", user.nickname, user.user_id);
            printf("    Real Name: %s\n\n", user.real_name ? "✓" : "✗");
            need_re_auth = 0;

            // (保持 g_config 中的用户信息最新)
            config_update_auth_from_user(&user);
        } else {
            log_warn("Existing token is invalid for AGC (AppGallery Connect). Forcing re-authentication...\n");
            memset(&user, 0, sizeof(user));
        }

        if (device_ids) {
            for (int i = 0; i < device_count; i++) {
                free(device_ids[i]);
            }
            free(device_ids);
        }
        if (device_names) {
            for (int i = 0; i < device_count; i++) {
                free(device_names[i]);
            }
            free(device_names);
        }
    }
    
    if (need_re_auth) {
        if (auth_init_oauth(&user) != 0) {
            return 1;
        }
        
        log_info("Authentication successful!");
        printf("    User: %s (%s)\n", user.nickname, user.user_id);
        printf("    Real Name: %s\n\n", user.real_name ? "✓" : "✗");
        
        // (重构: 保存到 g_config)
        config_update_auth_from_user(&user);
        
    }
    
    // (统一保存)
    if (config_save() != 0) {
        log_warn("Failed to save authentication token to config.json.");
    }
    
    
    // ===== 阶段 4: 生成密钥和证书 (Robust Logic) =====
    log_info("Step 2: Signing Key & Certificate Setup");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    char *keystore_path = get_signature_path("horpkg.p12");
    char *cert_path = get_signature_path("horpkg.cer");
    char *csr_path = get_signature_path("horpkg.csr");
    
    // local_cert 跟踪本地状态, cloud_cert 跟踪云端状态
    cert_info_t cloud_cert = {0}; 
    cert_info_t local_cert = {0}; 
    
    int needs_p12_generation = 0;
    int needs_csr_request = 0;
    
    // 1. 检查本地 P12 (Keystore)
    int local_p12_exists = (access(keystore_path, F_OK) == 0);
    
    // 2. 检查本地 ID (从 g_config 读取)
    if (g_config.cert_id[0] != '\0') {
        strncpy(local_cert.id, g_config.cert_id, sizeof(local_cert.id) - 1);
    }
    int local_id_exists = (local_cert.id[0] != '\0');

    // 3. 检查云端证书 (API 3)
    // (构建我们期望的证书名称)
    char cert_name_to_find[128];
    snprintf(cert_name_to_find, sizeof(cert_name_to_find), "horpkg_auto_%s.cer", user.user_id);
    
    int cloud_cert_exists = (signing_get_cert_list_and_find(&user, cert_name_to_find, &cloud_cert) == 0);
    if (!cloud_cert_exists) {
        // 如果没找到新版名称，尝试查找旧版 "horpkg"
        log_info("Checking for legacy 'horpkg' certificate name...");
        cloud_cert_exists = (signing_get_cert_list_and_find(&user, "horpkg", &cloud_cert) == 0);
    }
    
    // --- 4. 执行用户定义的4种场景逻辑 ---

    if (local_p12_exists) {
        if (local_id_exists) {
            if (!cloud_cert_exists) {
                // 场景 1: 本地有签名、ID，云端无签名
                log_warn("Local P12 and ID exist, but no matching certificate found on cloud.");
                log_info("Using local P12 (keystore) to request a new certificate.");
                needs_csr_request = 1; // (P12 exists, no new P12 needed)
            } else {
                // (隐式场景): 本地有 P12, 本地有 ID, 云端有 ID
                if (strcmp(local_cert.id, cloud_cert.id) != 0) {
                    log_warn("Local cert ID does not match cloud cert ID. Using cloud version.");
                }
                log_info("Local P12 and Cloud certificate are in sync.");
                local_cert = cloud_cert; // 确保 local_cert 持有云端的有效数据
                needs_csr_request = 0;
                needs_p12_generation = 0;
            }
        } else { // (local_p12_exists && !local_id_exists)
            if (!cloud_cert_exists) {
                // 场景 2: 本地有签名、无ID，云端无签名
                log_warn("Local P12 exists, but no local ID or cloud certificate found.");
                log_info("Using local P12 (keystore) to request a new certificate.");
                needs_csr_request = 1;
            } else {
                // 场景 3: 本地有签名、无ID，云端有签名
                log_info("Local P12 exists, local ID was missing.");
                log_info("Successfully recovered certificate ID from cloud.");
                local_cert = cloud_cert; // 恢复 ID
                needs_csr_request = 0;
            }
        }
    } else { // (!local_p12_exists)
        // 场景 4: 本地无签名
        log_warn("Local P12 keystore ('horpkg.p12') not found.");
        if (cloud_cert_exists) {
            // "如果云端有签名就删除"
            log_warn("Found an existing certificate ('%s') on cloud without a local P12.", cloud_cert.name);
            log_info("Deleting cloud certificate to ensure consistency... (API 4)");
            if (signing_delete_cert(&user, cloud_cert.id) != 0) {
                log_error("Failed to delete existing cloud certificate. Please delete it manually via AGConnect.");
                free(keystore_path); free(cert_path);
                return 1; 
            }
        }
        // "本地重新生成P12、CSR申请签名"
        log_info("Generating new P12 keystore...");
        needs_p12_generation = 1;
        needs_csr_request = 1;
    }

    // --- 5. 执行操作 (生成/请求) ---
    if (needs_p12_generation) {
        if (signing_generate_keystore(keystore_path, "horpkg", "horpkg") != 0) {
            log_error("Failed to generate keystore");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        log_info("Keystore created.\n");
    }
    
    if (needs_csr_request) {
        log_info("Generating CSR from keystore...");
        char csr[4096];
        
        if (signing_generate_csr(keystore_path, "horpkg", "horpkg", csr_path, csr, sizeof(csr)) != 0) {
            log_error("Failed to generate CSR");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        log_info("CSR generated and saved to: %s\n", csr_path);
        
        log_info("Requesting certificate from Huawei Cloud (API 5)...");
        if (signing_request_cert(&user, csr, &local_cert) != 0) {
            log_error("Failed to request certificate");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        log_info("Certificate created:");
        printf("    ID: %s\n\n", local_cert.id);
        
        log_info("Downloading certificate (API 6.1)...");
        if (signing_download_cert(local_cert.object_id, &user, cert_path) != 0) {
            log_error("Failed to download certificate");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        log_info("Certificate downloaded and saved to: %s\n", cert_path);
    } 
    
    else if (cloud_cert_exists && access(cert_path, F_OK) != 0) {
        log_warn("Local .cer file is missing. Downloading existing cloud certificate...");
        if (signing_download_cert(cloud_cert.object_id, &user, cert_path) != 0) {
            log_error("Failed to download existing certificate.");
        } else {
            log_info("Certificate downloaded and saved to: %s\n", cert_path);
        }
    }
    if (!needs_csr_request && local_p12_exists && access(csr_path, F_OK) != 0) {
        log_warn("Local .csr file is missing. Re-generating from existing keystore...");
        char csr_buffer_temp[4096]; 
        
        if (signing_generate_csr(keystore_path, "horpkg", "horpkg", 
                                 csr_path, csr_buffer_temp, sizeof(csr_buffer_temp)) != 0) {
            log_error("Failed to re-generate CSR.");
        } else {
            log_info("CSR successfully re-generated and saved to: %s\n", csr_path);
        }
    }
    // --- 6. 保存状态 ---
    // (重构: 保存到 g_config)
    if (local_cert.id[0] != '\0') {
        strncpy(g_config.cert_id, local_cert.id, sizeof(g_config.cert_id) - 1);
        if (config_save() != 0) {
            log_warn("Failed to write/update cert_id in config.json");
        }
    } else {
        // (如果执行到这里 local_cert.id 仍然为空，说明逻辑有严重错误)
        log_error("FATAL: Certificate ID is still empty after Step 2.");
        free(keystore_path); free(cert_path);
        return 1;
    }

    free(keystore_path);
    free(cert_path);
    free(csr_path);

    // ===== 阶段 5: 配置 Provision =====
    log_info("Step 3: Provision Configuration");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    const char *runtime_bundle_name = "org.horpkg.runtime";
    char provision_path[512] = {0};
    int provision_ready = 0;
    
    log_info("Ensuring provision profile for runtime package (%s)...", runtime_bundle_name);

    if (signing_ensure_provision_for_bundle(&user, runtime_bundle_name, &local_cert, provision_path, sizeof(provision_path)) != 0) {
        print_error_fmt("Failed to ensure provision profile for %s.", runtime_bundle_name);
    } else {
        print_info_fmt("Runtime provision profile is ready at: %s", provision_path);
        provision_ready = 1;
    }
    
    printf("\n%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s✅ Horpkg initialization complete!%s\n\n", COLOR_GREEN, COLOR_RESET);
    
    printf("Configuration Summary:\n");
    printf("  User: %s%s%s (%s)\n", COLOR_BOLD, user.nickname, COLOR_RESET, user.user_id);
    printf("  Keystore:    ~/.horpkg/signature/horpkg.p12\n");
    printf("  Certificate: ~/.horpkg/signature/horpkg.cer\n");
    printf("  CSR:         ~/.horpkg/signature/horpkg.csr\n");
    printf("  Provision:   %s\n", provision_ready ? provision_path : "(unavailable)");
    printf("  Runtime:     org.horpkg.runtime signing assets prepared (manual install)\n");
    printf("\n");
    
    printf("Next steps:\n");
    printf("  %shorpkg search python%s     # Search for packages\n", COLOR_CYAN, COLOR_RESET);
    printf("  %shorpkg install python%s    # Install a package\n", COLOR_CYAN, COLOR_RESET);
    printf("  %shorpkg list%s              # List installed packages\n\n", COLOR_CYAN, COLOR_RESET);
    
    return 0;
}
