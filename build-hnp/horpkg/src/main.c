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

int main(int argc, char *argv[]) {
    // No command provided
    if (argc < 2) {
        cmd_help(0, NULL);
        return 1;
    }
    
    const char *command = argv[1];
    
    // Find and execute command
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            return commands[i].handler(argc - 2, argv + 2);
        }
    }
    
    // Unknown command
    print_error("Unknown command");
    printf("Run '%shorpkg help%s' for usage.\n", COLOR_CYAN, COLOR_RESET);
    return 1;
}