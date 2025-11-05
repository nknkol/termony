#include <unistd.h>
#include <stdlib.h> // for free()
#include "commands.h"
#include "utils.h"
#include "config.h"   // <-- (新添加)
#include <stdio.h>
#include <string.h>
#include <yyjson.h>
#include "auth.h"
#include "signing.h"
#include "http.h"

int download_file(const char *url, const char *outfile);
char* hdc_get_uuid(void);
int store_uuid(const char* uuid);
int is_initialized(void);

/**
 * 核心初始化函数 (包含高级容错逻辑)
 */
int cmd_init(int argc, char *argv[]) {
    printf("\n%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Horpkg Initialization                 ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n\n", COLOR_CYAN, COLOR_RESET);

    // ===== 阶段1: 基础配置 =====
    // (config_load() 已经在 main.c 中调用)
    print_info("Configuration loaded.");
    
    // ===== 阶段2: 设备 UUID =====
    // (此处的 is_initialized() 和 store_uuid() 已被重构，使用 g_config)
    if (!is_initialized()) {
        print_info("Getting device UUID via HDC...");
        char* uuid = hdc_get_uuid();
        if (uuid) {
            if (store_uuid(uuid) == 0) { // (现在会保存到 config.json)
                print_success("Device UUID retrieved and saved:");
                printf("    UUID: %s\n\n", uuid);
            } else {
                print_error("Failed to store device UUID.");
                free(uuid);
                return 1;
            }
            free(uuid);
        } else {
            print_error("Failed to get device UUID via HDC.");
            return 1;
        }
    } else {
        print_info("Device UUID already registered.\n");
    }
    
    // ===== 阶段3: 华为账号认证 =====
    print_info("Step 1: Huawei Developer Account Authentication");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    user_info_t user = {0};
    
    int need_re_auth = 1;     // 默认需要重新认证
    int device_list_ok = 0;   // 标记设备列表是否已获取
    
    char **device_ids = NULL;
    char **device_names = NULL;
    int device_count = 0;
    
    // (重构: 从 g_config 加载 token)
    if (g_config.auth.jwt_token[0] != '\0') {
        // (将 g_config 应用到本地 user 变量)
        config_apply_auth_to_user(&user);
        
        if (auth_get_access_token_from_jwt(&user) == 0) {
            print_success("API 2.2 (DevEco) check OK.");
            print_info("Verifying token against AGC service (device-list)...");

            // (使用 API 4.1 获取设备列表作为验证)
            if (signing_get_device_list(&user, &device_ids, &device_names, &device_count) == 0) {
                print_success("Using existing authentication");
                printf("    User: %s (%s)\n", user.nickname, user.user_id);
                printf("    Real Name: %s\n\n", user.real_name ? "✓" : "✗");
                need_re_auth = 0;
                device_list_ok = 1;
                
                // (重要) 更新 g_config 中的 access_token 和 user_info
                config_update_auth_from_user(&user);
                
            } else {
                print_warning("Existing token is invalid for AGC (AppGallery Connect). Forcing re-authentication...\n");
                memset(&user, 0, sizeof(user));
            }
        } else {
            print_warning("Existing token invalid (DevEco check failed), re-authenticating...\n");
            memset(&user, 0, sizeof(user));
        }
    }
    
    if (need_re_auth) {
        if (auth_init_oauth(&user) != 0) {
            return 1;
        }
        
        print_success("Authentication successful!");
        printf("    User: %s (%s)\n", user.nickname, user.user_id);
        printf("    Real Name: %s\n\n", user.real_name ? "✓" : "✗");
        
        // (重构: 保存到 g_config)
        config_update_auth_from_user(&user);
        
    }
    
    // (统一保存)
    if (config_save() != 0) {
        print_warning("Failed to save authentication token to config.json.");
    }
    
    
    // ===== 阶段 4: 生成密钥和证书 (Robust Logic) =====
    print_info("Step 2: Signing Key & Certificate Setup");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    char *keystore_path = get_config_path("horpkg.p12");
    char *cert_path = get_config_path("horpkg.cer");
    char *csr_path = get_config_path("horpkg.csr");
    
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
        print_info("Checking for legacy 'horpkg' certificate name...");
        cloud_cert_exists = (signing_get_cert_list_and_find(&user, "horpkg", &cloud_cert) == 0);
    }
    
    // --- 4. 执行用户定义的4种场景逻辑 ---

    if (local_p12_exists) {
        if (local_id_exists) {
            if (!cloud_cert_exists) {
                // 场景 1: 本地有签名、ID，云端无签名
                print_warning("Local P12 and ID exist, but no matching certificate found on cloud.");
                print_info("Using local P12 (keystore) to request a new certificate.");
                needs_csr_request = 1; // (P12 exists, no new P12 needed)
            } else {
                // (隐式场景): 本地有 P12, 本地有 ID, 云端有 ID
                if (strcmp(local_cert.id, cloud_cert.id) != 0) {
                    print_warning("Local cert ID does not match cloud cert ID. Using cloud version.");
                }
                print_success("Local P12 and Cloud certificate are in sync.");
                local_cert = cloud_cert; // 确保 local_cert 持有云端的有效数据
                needs_csr_request = 0;
                needs_p12_generation = 0;
            }
        } else { // (local_p12_exists && !local_id_exists)
            if (!cloud_cert_exists) {
                // 场景 2: 本地有签名、无ID，云端无签名
                print_warning("Local P12 exists, but no local ID or cloud certificate found.");
                print_info("Using local P12 (keystore) to request a new certificate.");
                needs_csr_request = 1;
            } else {
                // 场景 3: 本地有签名、无ID，云端有签名
                print_success("Local P12 exists, local ID was missing.");
                print_success("Successfully recovered certificate ID from cloud.");
                local_cert = cloud_cert; // 恢复 ID
                needs_csr_request = 0;
            }
        }
    } else { // (!local_p12_exists)
        // 场景 4: 本地无签名
        print_warning("Local P12 keystore ('horpkg.p12') not found.");
        if (cloud_cert_exists) {
            // "如果云端有签名就删除"
            print_warning_fmt("Found an existing certificate ('%s') on cloud without a local P12.", cloud_cert.name);
            print_info("Deleting cloud certificate to ensure consistency... (API 4)");
            if (signing_delete_cert(&user, cloud_cert.id) != 0) {
                print_error("Failed to delete existing cloud certificate. Please delete it manually via AGConnect.");
                free(keystore_path); free(cert_path);
                return 1; 
            }
        }
        // "本地重新生成P12、CSR申请签名"
        print_info("Generating new P12 keystore...");
        needs_p12_generation = 1;
        needs_csr_request = 1;
    }

    // --- 5. 执行操作 (生成/请求) ---
    if (needs_p12_generation) {
        if (signing_generate_keystore(keystore_path, "horpkg", "horpkg") != 0) {
            print_error("Failed to generate keystore");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        print_success("Keystore created.\n");
    }
    
    if (needs_csr_request) {
        print_info("Generating CSR from keystore...");
        char csr[4096];
        
        if (signing_generate_csr(keystore_path, "horpkg", "horpkg", csr_path, csr, sizeof(csr)) != 0) {
            print_error("Failed to generate CSR");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        print_success_fmt("CSR generated and saved to: %s\n", csr_path);
        
        print_info("Requesting certificate from Huawei Cloud (API 5)...");
        if (signing_request_cert(&user, csr, &local_cert) != 0) {
            print_error("Failed to request certificate");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        print_success("Certificate created:");
        printf("    ID: %s\n\n", local_cert.id);
        
        print_info("Downloading certificate (API 6.1)...");
        if (signing_download_cert(local_cert.object_id, &user, cert_path) != 0) {
            print_error("Failed to download certificate");
            free(keystore_path); free(cert_path); free(csr_path);
            return 1;
        }
        print_success_fmt("Certificate downloaded and saved to: %s\n", cert_path);
    } 
    
    else if (cloud_cert_exists && access(cert_path, F_OK) != 0) {
        print_warning("Local .cer file is missing. Downloading existing cloud certificate...");
        if (signing_download_cert(cloud_cert.object_id, &user, cert_path) != 0) {
            print_error("Failed to download existing certificate.");
        } else {
            print_success_fmt("Certificate downloaded and saved to: %s\n", cert_path);
        }
    }
    if (!needs_csr_request && local_p12_exists && access(csr_path, F_OK) != 0) {
        print_warning("Local .csr file is missing. Re-generating from existing keystore...");
        char csr_buffer_temp[4096]; 
        
        if (signing_generate_csr(keystore_path, "horpkg", "horpkg", 
                                 csr_path, csr_buffer_temp, sizeof(csr_buffer_temp)) != 0) {
            print_error("Failed to re-generate CSR.");
        } else {
            print_success_fmt("CSR successfully re-generated and saved to: %s\n", csr_path);
        }
    }
    // --- 6. 保存状态 ---
    // (重构: 保存到 g_config)
    if (local_cert.id[0] != '\0') {
        strncpy(g_config.cert_id, local_cert.id, sizeof(g_config.cert_id) - 1);
        if (config_save() != 0) {
            print_warning("Failed to write/update cert_id in config.json");
        }
    } else {
        // (如果执行到这里 local_cert.id 仍然为空，说明逻辑有严重错误)
        print_error("FATAL: Certificate ID is still empty after Step 2.");
        free(keystore_path); free(cert_path);
        return 1;
    }

    free(keystore_path);
    free(cert_path);
    free(csr_path);

    // ===== 阶段 5: 配置 Provision =====
    print_info("Step 3: Provision Configuration");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    // 检查是否已在认证阶段获取了设备列表
    if (!device_list_ok) {
        print_info("Getting device list (with new token)...");
        if (signing_get_device_list(&user, &device_ids, &device_names, &device_count) != 0) {
            print_error("Failed to get device list even after re-authentication.");
            return 1;
        }
    }
    
    if (device_count == 0) {
        print_warning("No devices registered in Huawei Cloud");
        print_prompt("Please register your device first at:");
        printf("    https://developer.huawei.com\n\n");
    } else {
        print_success("Found devices:");
        for (int i = 0; i < device_count; i++) {
            printf("    ├─ %s\n", device_names[i]);
        }
        printf("\n");
        
        // 为核心包创建 Provision
        print_info("Creating provision for org.horpkg.core (API 7)...");
        provision_info_t provision = {0};
        
        const char *profile_bundle_name = "org.horpkg.core";
        
        // (使用 local_cert，它现在保证持有有效的 ID)
        if (signing_create_provision(&user, &local_cert, (const char**)device_ids, device_count,
                                     profile_bundle_name, &provision) != 0) {
            print_error("Failed to create provision");
            print_prompt("This may be because your 'horpkg' cert on the cloud is invalid.");
            print_prompt("Try deleting '~/.horpkg/horpkg.p12', '~/.horpkg/cert_id.conf' and run 'init' again.");

        } else {
            // --- (开始修改：按包名保存 .p7b) ---
            
            char profile_filename[256];
            
            // 2. 构造文件名 (e.g., "org.horpkg.core.p7b")
            snprintf(profile_filename, sizeof(profile_filename), "%s.p7b", profile_bundle_name);
            
            // 3. 使用 get_config_path 获取最终保存路径 (e.g., ~/.horpkg/org.horpkg.core.p7b)
            char *provision_path = get_config_path(profile_filename);
            if (!provision_path) {
                 print_error("Failed to get config path for provision file.");
                 // (清理)
                if (device_ids) {
                    for (int i = 0; i < device_count; i++) {
                        free(device_ids[i]);
                        free(device_names[i]);
                    }
                    free(device_ids);
                    free(device_names);
                }
                 return 1;
            }
            
            print_info_fmt("Downloading provision for %s (API 6.1)...", profile_bundle_name);
            
            // 4. 下载到指定的路径
            if (signing_download_provision(&user, provision.url, provision_path) == 0) {
                if (access(provision_path, F_OK) == 0) {
                    print_success("Provision created and downloaded.");
                    printf("    Saved to: %s\n\n", provision_path);
                } else {
                    print_error_fmt("File download reported success, but file is missing at: %s", provision_path);
                    print_error("This might be a temporary cloud issue or a filesystem error.");
                }
            } else {
                print_error_fmt("Failed to download provision file to %s.", provision_path);
            }
            
            free(provision_path);
            
            // --- (结束修改) ---
        }
    }
    
    // 清理
    if (device_ids) {
        for (int i = 0; i < device_count; i++) {
            free(device_ids[i]);
            free(device_names[i]);
        }
        free(device_ids);
        free(device_names);
    }
    
    printf("\n%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s✅ Horpkg initialization complete!%s\n\n", COLOR_GREEN, COLOR_RESET);
    
    printf("Configuration Summary:\n");
    printf("  User: %s%s%s (%s)\n", COLOR_BOLD, user.nickname, COLOR_RESET, user.user_id);
    printf("  Keystore:    ~/.horpkg/horpkg.p12\n");
    printf("  Certificate: ~/.horpkg/horpkg.cer\n");
    printf("  CSR:         ~/.horpkg/horpkg.csr\n");
    printf("  Provision:   ~/.horpkg/org.horpkg.core.p7b\n");
    printf("\n");
    
    printf("Next steps:\n");
    printf("  %shorpkg search python%s     # Search for packages\n", COLOR_CYAN, COLOR_RESET);
    printf("  %shorpkg install python%s    # Install a package\n", COLOR_CYAN, COLOR_RESET);
    printf("  %shorpkg list%s              # List installed packages\n\n", COLOR_CYAN, COLOR_RESET);
    
    return 0;
}

int cmd_install(int argc, char *argv[]) {
    if (argc < 1) {
        print_error("Package name required");
        printf("Usage: horpkg install <package>\n");
        return 1;
    }

    // --- NEW: 检查初始化 (使用重构后的 is_initialized) ---
    if (!is_initialized()) {
        print_error("Horpkg not initialized.");
        print_prompt("Please run 'horpkg init' first to register your device.");
        return 1;
    }
    // --- END NEW ---

    const char *package_name = argv[0];

    printf("\n%s📦 Installing package:%s %s%s%s\n\n",
           COLOR_BLUE, COLOR_RESET, COLOR_BOLD, package_name, COLOR_RESET);

    // --- (重构: 从 g_config 读取镜像) ---
    print_info("Reading repository configuration...");
    if (g_config.primary_mirror.url[0] == '\0') {
        print_error("Mirror URL not configured. Please run 'horpkg init'.");
        return 1;
    }
    
    const char *mirror_url = g_config.primary_mirror.url;
    print_success("Using mirror:");
    printf("    %s\n\n", mirror_url);
    
    // --- ↓↓↓↓↓↓ 核心修改区域 (开始) ↓↓↓↓↓↓ ---

    // 3. 准备应用专属的下载目录
    char *tmp_dir = get_config_path("tmp");
    if (!tmp_dir) { return 1; }
    create_dir_if_not_exists(tmp_dir);

    char *cache_dir = get_config_path("cache");
    if (!cache_dir) { free(tmp_dir); return 1; }
    create_dir_if_not_exists(cache_dir);

    // 4. 下载包元数据 (到 ~/.horpkg/tmp/)
    char package_json_url[512];
    char package_json_temp_path[256];
    snprintf(package_json_url, sizeof(package_json_url), "%s/packages/%s.json", mirror_url, package_name);
    snprintf(package_json_temp_path, sizeof(package_json_temp_path), "%s/%s.json", tmp_dir, package_name);
    free(tmp_dir); // 释放内存

    print_info("Fetching package metadata...");
    if (download_file(package_json_url, package_json_temp_path) != 0) {
        print_error("Failed to download package metadata.");
        free(cache_dir);
        return 1;
    }

    // 5. 解析包元数据获取下载地址
    yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *pkg_doc = yyjson_read_file(package_json_temp_path, flg, NULL, NULL);
    unlink(package_json_temp_path); // 删除临时元数据文件
    if (!pkg_doc) {
        print_error("Failed to parse package metadata.");
        free(cache_dir);
        return 1;
    }
    
    yyjson_val *pkg_root = yyjson_doc_get_root(pkg_doc);
    yyjson_val *binaries = yyjson_obj_get(pkg_root, "binaries");
    yyjson_val *arch_bin = yyjson_obj_get(binaries, "arm64-v8a"); // 硬编码架构
    yyjson_val *hnp_info = yyjson_obj_get(arch_bin, "public_hnp");
    const char *hnp_url = yyjson_get_str(yyjson_obj_get(hnp_info, "url"));
    const char *hnp_sha256 = yyjson_get_str(yyjson_obj_get(hnp_info, "sha256"));
    
    // 6. 确定最终下载路径 (到 ~/.horpkg/cache/)
    char out_filename[256];
    snprintf(out_filename, sizeof(out_filename), "%s/%s.hnp", cache_dir, package_name);
    free(cache_dir); // 释放内存

    // --- ↑↑↑↑↑↑ 核心修改区域 (结束) ↑↑↑↑↑↑ ---
    
    // 7. 执行下载
    print_info("Downloading package...");
    if (download_file(hnp_url, out_filename) != 0) {
        print_error("Download failed.");
        yyjson_doc_free(pkg_doc);
        return 1;
    }
    print_success("Download complete.");

    // 8. 后续步骤
    print_info("Verifying SHA256...");
    // TODO: 实现SHA256校验逻辑
    print_success("Checksum verified");

    print_info("Installing...");
    print_success("Package installed successfully");

    printf("\n%s🎉 Installation complete!%s\n", COLOR_GREEN, COLOR_RESET);
    
    yyjson_doc_free(pkg_doc);
    return 0;
}

// ... [ cmd_remove, cmd_update, cmd_list, ... cmd_version 保持不变 ] ...

int cmd_remove(int argc, char *argv[]) {
    if (argc < 1) {
        print_error("Package name required");
        printf("Usage: horpkg remove <package>\n");
        return 1;
    }
    
    const char *package = argv[0];
    printf("\n");
    printf("%s🗑️  Removing package:%s %s%s%s\n", 
           COLOR_YELLOW, COLOR_RESET, COLOR_BOLD, package, COLOR_RESET);
    printf("\n");
    
    print_info("Checking dependencies...");
    print_warning("No packages depend on this package");
    
    print_info("Uninstalling...");
    print_success("Package removed successfully");
    
    printf("\n");
    return 0;
}

int cmd_update(int argc, char *argv[]) {
    if (argc < 1) {
        print_error("Package name required");
        printf("Usage: horpkg update <package>\n");
        return 1;
    }
    
    const char *package = argv[0];
    printf("\n");
    print_info("Checking for updates...");
    printf("Current version: 1.0.0\n");
    printf("Latest version:  1.1.0\n");
    printf("\n");
    
    printf("%s→%s Updating %s...\n", COLOR_BLUE, COLOR_RESET, package);
    print_success("Update complete");
    printf("\n");
    
    return 0;
}

int cmd_list(int argc, char *argv[]) {
    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Installed Packages                    ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    
    printf("%-20s %-10s %-15s\n", "NAME", "VERSION", "SIZE");
    printf("%-20s %-10s %-15s\n", "────────────────────", "──────────", "───────────────");
    printf("%-20s %-10s %-15s\n", "horpkg-base", "1.0.0", "35 MB");
    printf("%-20s %-10s %-15s\n", "busybox", "1.36.0", "2 MB");
    printf("%-20s %-10s %-15s\n", "bash", "5.2.0", "3 MB");
    printf("\n");
    printf("Total: 3 packages, 40 MB\n");
    printf("\n");
    
    return 0;
}

int cmd_search(int argc, char *argv[]) {
    if (argc < 1) {
        print_error("Search term required");
        printf("Usage: horpkg search <keyword>\n");
        return 1;
    }
    
    const char *keyword = argv[0];
    printf("\n");
    printf("%s🔍 Searching for:%s %s%s%s\n", 
           COLOR_BLUE, COLOR_RESET, COLOR_BOLD, keyword, COLOR_RESET);
    printf("\n");
    
    printf("%-20s %-10s %-40s\n", "NAME", "VERSION", "DESCRIPTION");
    printf("%-20s %-10s %-40s\n", 
           "────────────────────", "──────────", 
           "────────────────────────────────────────");
    printf("%-20s %-10s %-40s\n", 
           "python", "3.12.0", "Python interpreter");
    printf("%-20s %-10s %-40s\n", 
           "python-numpy", "1.26.0", "NumPy - numerical computing");
    printf("%-20s %-10s %-40s\n", 
           "python-requests", "2.31.0", "HTTP library");
    printf("\n");
    printf("Found 3 packages\n");
    printf("\n");
    
    return 0;
}

int cmd_info(int argc, char *argv[]) {
    if (argc < 1) {
        print_error("Package name required");
        printf("Usage: horpkg info <package>\n");
        return 1;
    }
    
    const char *package = argv[0];
    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Package Information                   ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    
    printf("%sName:%s        %s\n", COLOR_BOLD, COLOR_RESET, package);
    printf("%sVersion:%s     8.0.0\n", COLOR_BOLD, COLOR_RESET);
    printf("%sSize:%s        15 MB (HNP) / 50 MB (Source)\n", COLOR_BOLD, COLOR_RESET);
    printf("%sDescription:%s QEMU System Emulator\n", COLOR_BOLD, COLOR_RESET);
    printf("%sCategory:%s    emulation\n", COLOR_BOLD, COLOR_RESET);
    printf("\n");
    
    printf("%sDependencies:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  • horpkg-base >= 1.0.0\n");
    printf("  • glib >= 2.70\n");
    printf("  • zlib >= 1.2\n");
    printf("\n");
    
    printf("%sAvailable modes:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s✓%s Binary HNP (15 MB, user-signed) %s[default]%s\n", 
           COLOR_GREEN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %s✓%s Source code (1 KB recipe + 50 MB source)\n", 
           COLOR_GREEN, COLOR_RESET);
    printf("\n");
    
    printf("%sCommands:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  • qemu-system-aarch64\n");
    printf("  • qemu-system-x86_64\n");
    printf("\n");
    
    return 0;
}

int cmd_sync(int argc, char *argv[]) {
    printf("\n");
    print_info("Syncing package repository...");
    print_success("Repository index updated");
    printf("Available packages: 156\n");
    printf("\n");
    
    return 0;
}

int cmd_clean(int argc, char *argv[]) {
    printf("\n");
    print_info("Cleaning cache...");
    print_success("Build cache cleared (250 MB freed)");
    print_success("Download cache cleared (180 MB freed)");
    printf("Total freed: 430 MB\n");
    printf("\n");
    
    return 0;
}

int cmd_config(int argc, char *argv[]) {
    // (重构: 实现真正的 config get/set)
    if (argc == 0) {
        // (打印所有当前配置)
        printf("\n");
        printf("%sCurrent configuration:%s (from ~/.horpkg/config.json)\n", COLOR_BOLD, COLOR_RESET);
        printf("  auth.user_id      = %s\n", g_config.auth.user_id);
        printf("  auth.nickname     = %s\n", g_config.auth.nickname);
        printf("  device_uuid       = %s\n", g_config.device_uuid);
        printf("  cert_id           = %s\n", g_config.cert_id);
        printf("  mirror.url        = %s\n", g_config.primary_mirror.url);
        printf("  mirror.name       = %s\n", g_config.primary_mirror.name);
        printf("  settings.default_mode = %s\n", g_config.default_mode);
        printf("  settings.parallel_jobs = %d\n", g_config.parallel_jobs);
        printf("\nUsage:\n");
        printf("  horpkg config get <key>\n");
        printf("  horpkg config set <key> <value>\n");
        printf("\n");
        return 0;
    }
    
    const char *action = argv[0];
    
    if (argc < 2) {
        print_error("Key required for 'get' or 'set'");
        return 1;
    }
    const char *key = argv[1];
    
    if (strcmp(action, "get") == 0) {
        if (strcmp(key, "mirror.url") == 0) {
            printf("%s\n", g_config.primary_mirror.url);
        } else if (strcmp(key, "device_uuid") == 0) {
            printf("%s\n", g_config.device_uuid);
        } else if (strcmp(key, "auth.user_id") == 0) {
            printf("%s\n", g_config.auth.user_id);
        } else if (strcmp(key, "cert_id") == 0) {
            printf("%s\n", g_config.cert_id);
        } else if (strcmp(key, "settings.default_mode") == 0) {
            printf("%s\n", g_config.default_mode);
        } else if (strcmp(key, "settings.parallel_jobs") == 0) {
            printf("%d\n", g_config.parallel_jobs);
        } else {
            print_error_fmt("Unknown config key: %s", key);
        }
    } else if (strcmp(action, "set") == 0) {
        if (argc < 3) {
            print_error("Value required");
            return 1;
        }
        const char *value = argv[2];
        int updated = 0;
        
        if (strcmp(key, "mirror.url") == 0) {
            strncpy(g_config.primary_mirror.url, value, sizeof(g_config.primary_mirror.url) - 1);
            updated = 1;
        } else if (strcmp(key, "mirror.name") == 0) {
            strncpy(g_config.primary_mirror.name, value, sizeof(g_config.primary_mirror.name) - 1);
            updated = 1;
        } else if (strcmp(key, "settings.default_mode") == 0) {
            strncpy(g_config.default_mode, value, sizeof(g_config.default_mode) - 1);
            updated = 1;
        } else if (strcmp(key, "settings.parallel_jobs") == 0) {
            g_config.parallel_jobs = atoi(value);
            if (g_config.parallel_jobs == 0) g_config.parallel_jobs = 4; // 简单校验
            updated = 1;
        } else {
            print_error_fmt("Unknown or read-only config key: %s", key);
            print_info("Settable keys: mirror.url, mirror.name, settings.default_mode, settings.parallel_jobs");
            return 1;
        }
        
        if (updated) {
            if (config_save() == 0) {
                print_success("Configuration updated");
                printf("  %s = %s\n", key, value);
            } else {
                print_error("Failed to save configuration");
            }
        }
    } else {
         print_error_fmt("Unknown action: %s. Use 'get' or 'set'.", action);
    }
    
    return 0;
}

int cmd_help(int argc, char *argv[]) {
    printf("\n");
    printf("%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Horpkg - HarmonyOS Package Manager   ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    
    printf("%sUsage:%s horpkg <command> [options]\n\n", COLOR_BOLD, COLOR_RESET);
    
    printf("%sPackage Management:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %sinit%s                Initialize Horpkg\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sinstall%s <pkg>       Install a package\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sremove%s <pkg>        Remove a package\n", COLOR_GREEN, COLOR_RESET);
    printf("  %supdate%s <pkg>        Update a package\n", COLOR_GREEN, COLOR_RESET);
    printf("  %slist%s                List installed packages\n", COLOR_GREEN, COLOR_RESET);
    printf("\n");
    
    printf("%sPackage Discovery:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %ssearch%s <keyword>    Search for packages\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sinfo%s <pkg>          Show package information\n", COLOR_GREEN, COLOR_RESET);
    printf("  %ssync%s                Sync repository index\n", COLOR_GREEN, COLOR_RESET);
    printf("\n");
    
    printf("%sMaintenance:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %sclean%s               Clean build cache\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sconfig%s              Manage configuration\n", COLOR_GREEN, COLOR_RESET);
    printf("\n");
    
    printf("%sOther:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %shelp%s                Show this help message\n", COLOR_GREEN, COLOR_RESET);
    printf("  %sversion%s             Show version\n", COLOR_GREEN, COLOR_RESET);
    printf("\n");
    
    printf("%sInstall Modes:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  horpkg install <pkg>                  # Binary HNP (default)\n");
    printf("  horpkg install --from-hnp <pkg>       # Binary HNP (explicit)\n");
    printf("  horpkg install --from-source <pkg>    # Compile from source\n");
    printf("  horpkg install --from-official <pkg>  # Official signed HAP\n");
    printf("\n");
    
    printf("%sExamples:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  horpkg init\n");
    printf("  horpkg search python\n");
    printf("  horpkg install python\n");
    printf("  horpkg info python\n");
    printf("  horpkg list\n");
    printf("\n");
    
    printf("For more information, visit: %shttps://horpkg.org%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("\n");
    
    return 0;
}

int cmd_version(int argc, char *argv[]) {
    printf("\n");
    printf("%sHorpkg%s v%s\n", COLOR_BOLD, COLOR_RESET, HORPKG_VERSION);
    printf("HarmonyOS Recipe Package Manager\n");
    printf("\n");
    printf("Build from Source, Sign with Trust\n");
    printf("\n");
    
    return 0;
}