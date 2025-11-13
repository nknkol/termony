/*
 * =================================================================
 * 通用 C Shim (Wrapper) - shim.c
 * (适配自动化友好的 config.h)
 * =================================================================
 *
 * 编译:
 * gcc -O2 -o my_shim shim.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// 包含你的标准化头文件
#include "config.h"

/*
 * -----------------------------------------------------------------
 * X-Macro 魔法开始
 * -----------------------------------------------------------------
 * 我们将 config.h 中定义的 SHIM_ENV_VARS 宏转换为一个静态数组
 */

// 1. 首先，定义 'X' 宏的“实现”，将其展开为 C 结构体数组的成员
#define X(name, value) { name, value },

// 2. 然后，定义我们的静态常量数组
static const struct {
    const char *name;
    const char *value;
} CUSTOM_ENV_VARS[] = {
    #ifdef SHIM_ENV_VARS
        SHIM_ENV_VARS  // 在这里，预处理器会将 SHIM_ENV_VARS 展开
                       // 例如：{ "PYTHONPATH", "/path" }, { "DEBUG", "0" },
    #endif
    { NULL, NULL }     // 数组的终止符
};

// 3. 立即取消定义 'X' 宏，防止污染后续代码
#undef X

/*
 * -----------------------------------------------------------------
 * 可选：命令行前缀 (例如 "-jar <file>")
 * -----------------------------------------------------------------
 */

#ifdef SHIM_EXTRA_ARGS
#define X(val) val,
static const char *EXTRA_ARGS[] = {
    SHIM_EXTRA_ARGS
    NULL
};
#undef X
#else
static const char *EXTRA_ARGS[] = { NULL };
#endif

/*
 * -----------------------------------------------------------------
 * X-Macro 魔法结束
 * -----------------------------------------------------------------
 */


// 辅助函数：安全地设置环境变量
static void set_env_var(const char *name, const char *value) {
    if (setenv(name, value, 1) != 0) {
        fprintf(stderr, "shim: Failed to set environment variable: %s\n", name);
        perror("shim: setenv error");
    }
}

// 核心功能：处理库路径
static void setup_library_path() {
    const char *custom_path = CUSTOM_LIB_PATH;
    const char *env_var_name = LIB_PATH_ENV_VAR;

    // 检查 config.h 是否定义了路径
    if (custom_path == NULL || custom_path[0] == '\0' || 
        env_var_name == NULL || env_var_name[0] == '\0') {
        return; // 没有定义，直接返回
    }

    const char *old_path = getenv(env_var_name);
    char *new_path = NULL;

    if (old_path && old_path[0] != '\0') {
        // 存在旧路径，需要合并 (格式: "NEW_PATH:OLD_PATH")
        size_t new_len = strlen(custom_path) + strlen(old_path) + 2; // +2 for ':' and '\0'
        new_path = (char *)malloc(new_len);
        
        if (new_path == NULL) {
            perror("shim: malloc failed for library path");
            exit(126); // 内存不足
        }
        snprintf(new_path, new_len, "%s:%s", custom_path, old_path);
    }

    // 设置环境变量
    set_env_var(env_var_name, (new_path != NULL) ? new_path : custom_path);

    if (new_path != NULL) {
        free(new_path);
    }
}

static size_t count_extra_args(void) {
    size_t count = 0;
    while (EXTRA_ARGS[count] != NULL) {
        count++;
    }
    return count;
}

static char **build_exec_argv(int argc, char *argv[]) {
    size_t extra = count_extra_args();
    size_t total = 1 + extra + (argc > 1 ? (argc - 1) : 0);
    char **new_argv = (char **)malloc((total + 1) * sizeof(char *));
    if (new_argv == NULL) {
        perror("shim: malloc failed for argv");
        exit(126);
    }

    size_t idx = 0;
    new_argv[idx++] = TARGET_COMMAND_PATH;
    for (size_t i = 0; i < extra; ++i) {
        new_argv[idx++] = (char *)EXTRA_ARGS[i];
    }
    for (int i = 1; i < argc; ++i) {
        new_argv[idx++] = argv[i];
    }
    new_argv[idx] = NULL;
    return new_argv;
}

int main(int argc, char *argv[]) {
    // 1. 设置库路径 (从 config.h 读取)
    setup_library_path();

    // 2. 循环设置所有自定义环境变量 (由 X-Macro 构建的数组)
    for (int i = 0; CUSTOM_ENV_VARS[i].name != NULL; i++) {
        set_env_var(CUSTOM_ENV_VARS[i].name, CUSTOM_ENV_VARS[i].value);
    }

    // 3. 构造新的 argv (可插入 -jar 等前缀) 并执行目标命令
    char **exec_argv = build_exec_argv(argc, argv);
    execv(exec_argv[0], exec_argv);

    // --- 如果 execv 成功，代码永远不会执行到这里 ---

    fprintf(stderr, "shim: FATAL: Failed to execute target command: %s\n", TARGET_COMMAND_PATH);
    perror("shim: execv error");
    free(exec_argv);
    
    return (errno == ENOENT) ? 127 : 126;
}
