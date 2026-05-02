#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "utils.h"
#include "config.h"
#include "logger.h"
#include <strings.h>

typedef struct {
    const char *name;
    int (*handler)(int argc, char *argv[]);
    const char *description;
} Command;

static const Command commands[] = {
    {"init",    cmd_init,    "Initialize Horpkg"},
    {"install", cmd_install, "Install a local HAP/HSP"},
    {"help",    cmd_help,    "Show help message"},
    {"version", cmd_version, "Show version"},
    {NULL, NULL, NULL}
};

void hdc_start_service(void);
int hdc_is_connected(void);
int hdc_connect_port(const char *port);
void print_prompt(const char *msg);

static int attempt_connect_with_port(const char *port, int announce_cached) {
    if (!port || port[0] == '\0') {
        return 0;
    }

    if (announce_cached) {
        print_info_fmt("Trying cached HDC port: %s", port);
    }

    if (hdc_connect_port(port) != 0) {
        return 0;
    }

    if (!hdc_is_connected()) {
        return 0;
    }

    print_success("Device connected successfully.");
    return 1;
}

static void persist_hdc_port_if_needed(const char *port) {
    if (!port || port[0] == '\0') {
        return;
    }

    if (strncmp(g_config.last_hdc_port, port, sizeof(g_config.last_hdc_port)) == 0) {
        return; // already stored
    }

    strncpy(g_config.last_hdc_port, port, sizeof(g_config.last_hdc_port) - 1);
    g_config.last_hdc_port[sizeof(g_config.last_hdc_port) - 1] = '\0';
    if (config_save() != 0) {
        log_warn("Failed to persist cached HDC port.");
    }
}

int main(int argc, char *argv[]) {

    log_level_t level = LOG_LEVEL_INFO;
    const char *log_level_env = getenv("HORPKG_LOG_LEVEL");
    if (log_level_env) {
        if (strcasecmp(log_level_env, "DEBUG") == 0) {
            level = LOG_LEVEL_DEBUG;
        } else if (strcasecmp(log_level_env, "WARN") == 0) {
            level = LOG_LEVEL_WARN;
        } else if (strcasecmp(log_level_env, "ERROR") == 0) {
            level = LOG_LEVEL_ERROR;
        } else if (strcasecmp(log_level_env, "NONE") == 0) {
            level = LOG_LEVEL_NONE;
        }
    }
    logger_init(level);

    if (config_load() != 0) {
        log_fatal("Failed to load configuration. Exiting.");
        return 1;
    }

    if (argc < 2) {
        cmd_help(0, NULL);
        return 1;
    }
    
    // Parse global flags before command
    int arg_idx = 1;
    while (arg_idx < argc && argv[arg_idx][0] == '-') {
        if (strcmp(argv[arg_idx], "--manual-auth") == 0 || strcmp(argv[arg_idx], "--manual-login") == 0) {
            g_config.manual_auth = 1;
        } else {
            // Treat unknown flag as potentially part of command or error
            // For now, we assume strict "horpkg [global-flags] command" structure
            // But to be friendly, if we hit an unknown flag, we might stop parsing global flags
            // However, sticking to simple logic:
            if (strcmp(argv[arg_idx], "--help") == 0 || strcmp(argv[arg_idx], "-h") == 0) {
                cmd_help(0, NULL);
                return 0;
            } else if (strcmp(argv[arg_idx], "--version") == 0 || strcmp(argv[arg_idx], "-v") == 0) {
                cmd_version(0, NULL);
                return 0;
            }
             // Unknown flag, stop global parsing, let command handle it (or fail)
             break;
        }
        arg_idx++;
    }

    if (arg_idx >= argc) {
        cmd_help(0, NULL);
        return 1;
    }
    
    const char *command = argv[arg_idx];
    
    // --- MODIFIED HDC CONNECTION CHECK (REQ 1) ---
    
    // 仅 init / install 需要 HDC
    int requires_hdc = (strcmp(command, "init") == 0 || strcmp(command, "install") == 0);

    if (requires_hdc) {
        // 1. 尝试确保HDC服务正在运行 (根据用户要求)
        hdc_start_service(); 
        
        // 2. 检查连接状态
        if (!hdc_is_connected()) {
            int connected = 0;

            // 2.1 自动尝试使用缓存端口
            if (g_config.last_hdc_port[0] != '\0') {
                connected = attempt_connect_with_port(g_config.last_hdc_port, 1);
                if (!connected) {
                    log_warn("Cached HDC port failed. You'll be prompted for a new port.");
                }
            }

            if (!connected) {
                print_error("HDC device not connected.");
                print_prompt("Please connect your HarmonyOS device via HDC.");
                
                // Use the new setup connect function
                hdc_setup_connect();
                
                // Re-check connection
                if (!hdc_is_connected()) {
                     print_error("Device still not connected. Exiting.");
                     return 1;
                }
            }
        }
        // 如果执行到这里，说明设备已连接
    }
    // --- END MODIFIED HDC CONNECTION CHECK ---
    
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            // main.c中的HDC检查通过后，才执行命令
            return commands[i].handler(argc - (arg_idx + 1), argv + arg_idx + 1);
        }
    }
    
    // Unknown command
    print_error_fmt("Unknown command: %s", command);
    printf("Run '%shorpkg help%s' for usage.\n", COLOR_CYAN, COLOR_RESET);
    return 1;
}