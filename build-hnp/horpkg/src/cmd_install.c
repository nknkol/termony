#include "commands.h"
#include "utils.h"
#include "config.h"
#include "logger.h"
#include "download.h" // [!]
#include "install.h"  // [!]
#include "hap_parser.h" // [!]
#include "signing.h"  // [!]
#include "auth.h"     // [!]

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <yyjson.h>

// [!]
// [!] 这是 `install` 命令的主入口
// [!]
int cmd_install(int argc, char *argv[]) {
    if (argc < 1) {
        log_error("Package name or local HAP file path required.");
        printf("Usage: horpkg install <package_name>\n");
        printf("   or: horpkg install ./path/to/app.hap\n");
        return 1;
    }

    const char *package_arg = argv[0];

    // --- 检查初始化 (所有安装都需要) ---
    if (!is_initialized()) {
        log_error("Horpkg not initialized.");
        print_prompt("Please run 'horpkg init' first to register your device.");
        return 1;
    }

    if (auth_ensure_valid_session(NULL) != 0) {
        return 1;
    }

    // 检查是否为本地安装
    // (如果参数以 ./, ../, / 开头, 或以 .hap / .hsp 结尾, 则视为本地文件)
    if (strncmp(package_arg, "./", 2) == 0 ||
        strncmp(package_arg, "../", 3) == 0 ||
        strncmp(package_arg, "/", 1) == 0 ||
        (strlen(package_arg) > 4 && strcmp(package_arg + strlen(package_arg) - 4, ".hap") == 0) ||
        (strlen(package_arg) > 4 && strcmp(package_arg + strlen(package_arg) - 4, ".hsp") == 0)) 
    {
        
        print_info_fmt("Starting local file installation for: %s", package_arg);
        return install_local_hap(package_arg);
        
    } else {
        
        print_info_fmt("Starting repository installation for: %s", package_arg);
        return install_from_repository(package_arg);
    }
}

// [!]
// [!] (从旧的 cmd_install 移动而来) 仓库安装逻辑
// [!]
int install_from_repository(const char *package_name) {

    printf("\n%s📦 Installing package:%s %s%s%s\n\n",
           COLOR_BLUE, COLOR_RESET, COLOR_BOLD, package_name, COLOR_RESET);

    // --- (重构: 从 g_config 读取镜像) ---
    log_info("Reading repository configuration...");
    if (g_config.primary_mirror.url[0] == '\0') {
        log_error("Mirror URL not configured. Please run 'horpkg init'.");
        return 1;
    }
    
    const char *mirror_url = g_config.primary_mirror.url;
    log_info("Using mirror:");
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

    log_info("Fetching package metadata...");
    if (download_file(package_json_url, package_json_temp_path) != 0) {
        log_error("Failed to download package metadata.");
        free(cache_dir);
        return 1;
    }

    // 5. 解析包元数据获取下载地址
    yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *pkg_doc = yyjson_read_file(package_json_temp_path, flg, NULL, NULL);
    unlink(package_json_temp_path); // 删除临时元数据文件
    if (!pkg_doc) {
        log_error("Failed to parse package metadata.");
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
    log_info("Downloading package...");
    if (download_file(hnp_url, out_filename) != 0) {
        log_error("Download failed.");
        yyjson_doc_free(pkg_doc);
        return 1;
    }
    log_info("Download complete.");

    // 8. 后续步骤
    log_info("Verifying SHA256...");
    // TODO: 实现SHA256校验逻辑
    log_info("Checksum verified");

    // [!]
    // [!] 仓库安装 (HNP) 流程
    // [!]
    log_info("Repository HNP install logic not yet implemented.");
    log_warn("HNP downloaded, but signing and installation steps are pending.");
    // 1. 解压 HNP
    // 2. 找到 .hap / .hsp
    // 3. (循环) 对每个 HAP/HSP:
    //    a. 调用 install_local_hap(hap_path)
    // 4. 清理
    
    // 占位符：
    // snprintf(temp_hap_path, ...);
    // install_local_hap(temp_hap_path);


    printf("\n%s🎉 Installation complete!%s\n", COLOR_GREEN, COLOR_RESET);
    
    yyjson_doc_free(pkg_doc);
    return 0;
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
int install_local_hap(const char *hap_path) {
    
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

    // 3. 对 HAP 进行签名
    log_info("Step 2/3: Signing HAP...");

    char *tmp_dir = get_config_path("tmp");
    if (!tmp_dir) {
        log_error("Failed to get temporary directory path.");
        return -1;
    }
    create_dir_if_not_exists(tmp_dir); // 确保 tmp 目录存在

    char signed_hap_path[512];
    // (创建临时签名文件)
    snprintf(signed_hap_path, sizeof(signed_hap_path), "%s/horpkg_signed_%d.hap", tmp_dir, rand());
    free(tmp_dir); // 释放路径

    if (sign_hap(hap_path, bundle_name, signed_hap_path) != 0) {
        log_error("Failed to sign HAP file.");
        unlink(signed_hap_path); 
        return -1;
    }
    print_info_fmt("Temporary signed HAP created at: %s", signed_hap_path);

    // 4. 安装 HAP
    log_info("Step 3/3: Installing HAP via HDC...");
    int install_result = hdc_install_hap(signed_hap_path, bundle_name, local_install_prompt);

    // 5. 清理
    log_debug("Cleaning up temporary file: %s", signed_hap_path);
    unlink(signed_hap_path);
    
    if (install_result != 0) {
        log_error("HDC installation failed or was cancelled.");
        return -1;
    }

    print_success_fmt("Successfully installed local HAP: %s", hap_path);
    return 0;
}
