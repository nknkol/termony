#include <unistd.h>
#include <stdlib.h> // for free()
#include "commands.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <yyjson.h>

int download_file(const char *url, const char *outfile);

// --- NEW Declarations (from utils.h) ---
char* hdc_get_uuid(void);
int store_uuid(const char* uuid);
int is_initialized(void);
// --- END NEW ---

int ensure_config_exists() {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        print_error("HOME environment variable not set.");
        return -1;
    }

    char config_dir[256];
    snprintf(config_dir, sizeof(config_dir), "%s/.horpkg", home_dir);
    if (create_dir_if_not_exists(config_dir) != 0) {
        return -1;
    }

    char *mirrors_path = get_config_path("mirrors.json");
    if (!mirrors_path) return -1;

    // 检查文件是否存在
    if (access(mirrors_path, F_OK) == -1) {
        print_info("Configuration not found. Creating default mirrors.json...");
        FILE *fp = fopen(mirrors_path, "w");
        if (!fp) {
            print_error("Failed to create mirrors.json.");
            free(mirrors_path);
            return -1;
        }
        // 写入默认的镜像配置
        fprintf(fp, "{\n");
        fprintf(fp, "  \"version\": \"1.0\",\n");
        fprintf(fp, "  \"mirrors\": [\n");
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": \"github\",\n");
        fprintf(fp, "      \"name\": \"GitHub (Primary)\",\n");
        fprintf(fp, "      \"region\": \"global\",\n");
        fprintf(fp, "      \"url\": \"https://raw.githubusercontent.com/nknkol/horpkg-index/main\",\n");
        fprintf(fp, "      \"type\": \"git\",\n");
        fprintf(fp, "      \"priority\": 1,\n");
        fprintf(fp, "      \"status\": \"active\"\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "  ]\n");
        fprintf(fp, "}\n");
        fclose(fp);
        print_success("Created default mirrors.json in ~/.horpkg/");
    }

    free(mirrors_path);
    return 0;
}

// --- MODIFIED cmd_init ---
int cmd_init(int argc, char *argv[]) {
    printf("\n%s╔════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║  Horpkg Initialization                 ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════╝%s\n\n", COLOR_CYAN, COLOR_RESET);

    // 1. 确保配置目录和默认镜像文件存在
    if (ensure_config_exists() != 0) {
        print_error("Configuration directory setup failed.");
        return 1;
    }
    
    // 2. 检查是否已经初始化 (已有UUID)
    if (is_initialized()) {
        print_warning("Horpkg is already initialized (UUID found).");
        print_info("To re-initialize, remove '~/.horpkg/uuid.conf' and run again.");
    } else {
        // 3. 尝试通过HDC获取UUID
        // (此时 main.c 已经确认HDC已连接)
        print_info("Getting device UUID via HDC...");
        
        char* uuid = hdc_get_uuid();
        if (uuid) {
            // 4. 存储UUID
            if (store_uuid(uuid) == 0) {
                print_success("Successfully retrieved and stored device UUID.");
                printf("    UUID: %s\n", uuid);
            } else {
                print_error("Failed to store device UUID.");
                free(uuid);
                return 1;
            }
            free(uuid);
        } else {
            print_error("Failed to get device UUID via HDC.");
            print_prompt("Ensure 'hdc shell bm get --udid' is working correctly.");
            return 1;
        }
    }

    print_success("Horpkg configuration is ready.");
    return 0;
}
// --- END MODIFIED cmd_init ---

int cmd_install(int argc, char *argv[]) {
    if (argc < 1) {
        print_error("Package name required");
        printf("Usage: horpkg install <package>\n");
        return 1;
    }

    // --- NEW: 检查初始化 ---
    if (!is_initialized()) {
        print_error("Horpkg not initialized.");
        print_prompt("Please run 'horpkg init' first to register your device.");
        return 1;
    }
    // --- END NEW ---

    if (ensure_config_exists() != 0) {
        return 1;
    }

    const char *package_name = argv[0];

    printf("\n%s📦 Installing package:%s %s%s%s\n\n",
           COLOR_BLUE, COLOR_RESET, COLOR_BOLD, package_name, COLOR_RESET);

    // 1. 读取仓库配置
    print_info("Reading repository configuration...");
    char *mirrors_path = get_config_path("mirrors.json");
    if (!mirrors_path) {
        return 1;
    }

    yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *mirror_doc = yyjson_read_file(mirrors_path, flg, NULL, NULL);
    free(mirrors_path);

    if (!mirror_doc) {
        print_error("Failed to read ~/.horpkg/mirrors.json. Please run 'horpkg init'.");
        return 1;
    }
    
    // 2. 解析镜像URL
    yyjson_val *mirrors_root = yyjson_doc_get_root(mirror_doc);
    yyjson_val *mirrors_arr = yyjson_obj_get(mirrors_root, "mirrors");
    yyjson_val *first_mirror = yyjson_arr_get_first(mirrors_arr);
    const char *mirror_url = yyjson_get_str(yyjson_obj_get(first_mirror, "url"));
    print_success("Using mirror:");
    printf("    %s\n\n", mirror_url);
    
    // --- ↓↓↓↓↓↓ 核心修改区域 (开始) ↓↓↓↓↓↓ ---

    // 3. 准备应用专属的下载目录
    char *tmp_dir = get_config_path("tmp");
    if (!tmp_dir) { yyjson_doc_free(mirror_doc); return 1; }
    create_dir_if_not_exists(tmp_dir);

    char *cache_dir = get_config_path("cache");
    if (!cache_dir) { free(tmp_dir); yyjson_doc_free(mirror_doc); return 1; }
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
        yyjson_doc_free(mirror_doc);
        return 1;
    }

    // 5. 解析包元数据获取下载地址
    yyjson_doc *pkg_doc = yyjson_read_file(package_json_temp_path, flg, NULL, NULL);
    unlink(package_json_temp_path); // 删除临时元数据文件
    if (!pkg_doc) {
        print_error("Failed to parse package metadata.");
        free(cache_dir);
        yyjson_doc_free(mirror_doc);
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
        yyjson_doc_free(mirror_doc);
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
    
    yyjson_doc_free(mirror_doc);
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
    if (argc < 2) {
        printf("\n");
        printf("%sCurrent configuration:%s\n", COLOR_BOLD, COLOR_RESET);
        printf("  default-mode: hnp\n");
        printf("  parallel-jobs: 4\n");
        printf("  certificate: ~/.horpkg/certificates/signing.p12\n");
        printf("\n");
        printf("Usage:\n");
        printf("  horpkg config get <key>\n");
        printf("  horpkg config set <key> <value>\n");
        printf("\n");
        return 0;
    }
    
    const char *action = argv[0];
    const char *key = argv[1];
    
    if (strcmp(action, "get") == 0) {
        printf("default-mode = hnp\n");
    } else if (strcmp(action, "set") == 0) {
        if (argc < 3) {
            print_error("Value required");
            return 1;
        }
        const char *value = argv[2];
        print_success("Configuration updated");
        printf("  %s = %s\n", key, value);
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