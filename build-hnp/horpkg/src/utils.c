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