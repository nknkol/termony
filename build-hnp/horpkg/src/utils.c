#include "utils.h"
#include <stdio.h>

void print_success(const char *msg) {
    printf("%s✓%s %s\n", COLOR_GREEN, COLOR_RESET, msg);
}

void print_error(const char *msg) {
    fprintf(stderr, "%s✗ Error:%s %s\n", COLOR_RED, COLOR_RESET, msg);
}

void print_warning(const char *msg) {
    printf("%s⚠ Warning:%s %s\n", COLOR_YELLOW, COLOR_RESET, msg);
}

void print_info(const char *msg) {
    printf("%s→%s %s\n", COLOR_BLUE, COLOR_RESET, msg);
}

char* get_config_path(const char* filename) {
    const char *home_dir = getenv("HOME");
    if (!home_dir) {
        print_error("HOME environment variable not set.");
        return NULL;
    }
    char* path = malloc(strlen(home_dir) + strlen("/.horpkg/") + strlen(filename) + 1);
    if (path) {
        sprintf(path, "%s/.horpkg/%s", home_dir, filename);
    }
    return path;
}

int create_dir_if_not_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to create directory %s: %s", path, strerror(errno));
            print_error(err_msg);
            return -1;
        }
    }
    return 0;
}