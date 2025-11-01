#include "commands.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    int (*handler)(int argc, char *argv[]);
    const char *description;
} Command;

static const Command commands[] = {
    {"init",    cmd_init,    "Initialize Horpkg"},
    {"install", cmd_install, "Install a package"},
    {"remove",  cmd_remove,  "Remove a package"},
    {"update",  cmd_update,  "Update a package"},
    {"list",    cmd_list,    "List installed packages"},
    {"search",  cmd_search,  "Search for packages"},
    {"info",    cmd_info,    "Show package information"},
    {"sync",    cmd_sync,    "Sync repository index"},
    {"clean",   cmd_clean,   "Clean build cache"},
    {"config",  cmd_config,  "Manage configuration"},
    {"help",    cmd_help,    "Show help message"},
    {"version", cmd_version, "Show version"},
    {NULL, NULL, NULL}
};

// --- NEW Declarations (from utils.h) ---
void hdc_start_service(void);
int hdc_is_connected(void);
int hdc_connect_port(const char *port); // <-- 确保新函数被声明 (虽然在utils.h里)
void print_prompt(const char *msg);
// --- END NEW ---


int main(int argc, char *argv[]) {
    // No command provided
    if (argc < 2) {
        cmd_help(0, NULL);
        return 1;
    }
    
    const char *command = argv[1];
    
    // --- MODIFIED HDC CONNECTION CHECK (REQ 1) ---
    
    // 'help' 和 'version' 是唯一不需要HDC连接的命令
    int requires_hdc = 1;
    if (strcmp(command, "help") == 0 || strcmp(command, "version") == 0) {
        requires_hdc = 0;
    }

    if (requires_hdc) {
        // 1. 尝试确保HDC服务正在运行 (根据用户要求)
        hdc_start_service(); 
        
        // 2. 检查连接状态
        if (!hdc_is_connected()) {
            print_error("HDC device not connected.");
            print_prompt("Please connect your HarmonyOS device via HDC.");
            print_prompt("Check connection: 'hdc list targets'");
            
            // --- 新增的交互式连接逻辑 ---
            print_prompt("Enter <port> to connect (e.g., 38201) or press [Enter] to quit:");
            
            char port_input[32];
            if (fgets(port_input, sizeof(port_input), stdin) != NULL) {
                // 移除 fgets 带来的换行符
                port_input[strcspn(port_input, "\n")] = 0;
                
                // 检查用户是否输入了内容
                if (strlen(port_input) > 0) {
                    
                    if (hdc_connect_port(port_input) == 0) {
                        // Connect 命令已执行, 再次检查连接状态
                        if (!hdc_is_connected()) {
                            print_error("Connection attempt failed.");
                            print_prompt("Ensure the port is correct and hdc-lite is working.");
                            return 1; // 尝试连接后仍然失败，退出
                        }
                        print_success("Device connected successfully.");
                    } else {
                        // hdc_connect_port 返回错误 (例如 system() 失败)
                        print_error("Failed to execute connection command.");
                        return 1;
                    }
                } else {
                    // 用户直接按了回车
                    print_info("Connection cancelled by user. Exiting.");
                    return 1; 
                }
            } else {
                // fgets 读取失败
                print_error("Failed to read user input.");
                return 1;
            }
            // --- 交互式连接逻辑结束 ---

        }
        // 如果执行到这里，说明设备已连接
    }
    // --- END MODIFIED HDC CONNECTION CHECK ---
    
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            // main.c中的HDC检查通过后，才执行命令
            return commands[i].handler(argc - 2, argv + 2);
        }
    }
    
    // Unknown command
    print_error("Unknown command");
    printf("Run '%shorpkg help%s' for usage.\n", COLOR_CYAN, COLOR_RESET);
    return 1;
}