// build-hnp/horpkg/src/auth.h
#ifndef HORPKG_AUTH_H
#define HORPKG_AUTH_H

#include <stddef.h>  // ← 添加这一行，提供 size_t

// 用户信息结构体
typedef struct {
    char user_id[64];
    char nickname[128];
    char token[512];
    int real_name;
} user_info_t;

// 认证函数
int auth_init_oauth(user_info_t *user);
int auth_get_temp_token(char *token_out, size_t token_size);
int auth_check_jwt_token(const char *token, user_info_t *user);
int auth_get_user_info(const char *token, user_info_t *user);

#endif // HORPKG_AUTH_H