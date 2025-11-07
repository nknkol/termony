#ifndef HORPKG_AUTH_H
#define HORPKG_AUTH_H

#include <stddef.h>

typedef struct user_info_s { // (使用 user_info_s 方便 http.h 转发声明)
    char user_id[64];
    char nickname[128];
    char jwt_token[2048];     // (API 2.1) 用于换取 AccessToken
    char access_token[1024]; // (API 2.2) 用于所有后续的API调用
    char team_id[64];        // (API 1.2) 团队ID (通常等于 user_id)
    int real_name;           // (API 2.2) 是否实名
} user_info_t;

int auth_init_oauth(user_info_t *user);
// int auth_get_user_info(user_info_t *user);
int auth_get_access_token_from_jwt(user_info_t *user);
int auth_ensure_valid_session(user_info_t *user);


#endif // HORPKG_AUTH_H
