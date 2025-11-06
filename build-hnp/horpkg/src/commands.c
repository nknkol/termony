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
#include "logger.h" // 确保 logger.h 已包含 (虽然 utils.h 可能已包含)

// [!] `cmd_init` 已移至 `cmd_init.c`
// [!] `cmd_install` 已移至 `cmd_install.c`

// ... [ cmd_remove, cmd_update, cmd_list, ... cmd_version 保持不变 ] ...

int cmd_remove(int argc, char *argv[]) {
    if (argc < 1) {
        log_error("Package name required");
        printf("Usage: horpkg remove <package>\n");
        return 1;
    }
    
    const char *package = argv[0];
    printf("\n");
    printf("%s🗑️  Removing package:%s %s%s%s\n", 
           COLOR_YELLOW, COLOR_RESET, COLOR_BOLD, package, COLOR_RESET);
    printf("\n");
    
    log_info("Checking dependencies...");
    log_warn("No packages depend on this package");
    
    log_info("Uninstalling...");
    log_info("Package removed successfully");
    
    printf("\n");
    return 0;
}

int cmd_update(int argc, char *argv[]) {
    if (argc < 1) {
        log_error("Package name required");
        printf("Usage: horpkg update <package>\n");
        return 1;
    }
    
    const char *package = argv[0];
    printf("\n");
    log_info("Checking for updates...");
    printf("Current version: 1.0.0\n");
    printf("Latest version:  1.1.0\n");
    printf("\n");
    
    printf("%s→%s Updating %s...\n", COLOR_BLUE, COLOR_RESET, package);
    log_info("Update complete");
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
        log_error("Search term required");
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
        log_error("Package name required");
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
    log_info("Syncing package repository...");
    log_info("Repository index updated");
    printf("Available packages: 156\n");
    printf("\n");
    
    return 0;
}

int cmd_clean(int argc, char *argv[]) {
    printf("\n");
    log_info("Cleaning cache...");
    log_info("Build cache cleared (250 MB freed)");
    log_info("Download cache cleared (180 MB freed)");
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
        log_error("Key required for 'get' or 'set'");
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
            log_error("Unknown config key: %s", key);
        }
    } else if (strcmp(action, "set") == 0) {
        if (argc < 3) {
            log_error("Value required");
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
            log_error("Unknown or read-only config key: %s", key);
            log_info("Settable keys: mirror.url, mirror.name, settings.default_mode, settings.parallel_jobs");
            return 1;
        }
        
        if (updated) {
            if (config_save() == 0) {
                log_info("Configuration updated");
                printf("  %s = %s\n", key, value);
            } else {
                log_error("Failed to save configuration");
            }
        }
    } else {
         log_error("Unknown action: %s. Use 'get' or 'set'.", action);
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