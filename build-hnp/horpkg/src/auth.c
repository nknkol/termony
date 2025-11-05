#include "auth.h"
#include "http.h"
#include "http_server.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <yyjson.h>

#define AUTH_BASE "https://cn.devecostudio.huawei.com"

static void generate_random_code(char *code, size_t size) {
    FILE *fp = fopen("/dev/urandom", "rb");
    if (fp) {
        unsigned char random_bytes[16];
        fread(random_bytes, 1, 16, fp);
        fclose(fp);
        
        for (int i = 0; i < 16 && i*2 < size-1; i++) {
            sprintf(&code[i*2], "%02x", random_bytes[i]);
        }
        code[32] = '\0';
    } else {
        strcpy(code, "horpkg_default_code_12345678");
    }
}

static int auth_exchange_temp_for_jwt(const char *temp_token, user_info_t *user) {
    char url[2048];
    snprintf(url, sizeof(url),
             "%s/authrouter/auth/api/temptoken/check?site=CN&tempToken=%s&appid=1007&version=0.0.0",
             AUTH_BASE, temp_token);
    
    // (来自 API 2.1 规范的 Header)
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "user-agent: Dart/3.7 (dart:io)");
    headers = curl_slist_append(headers, "host: cn.devecostudio.huawei.com");
    headers = curl_slist_append(headers, "content-type: application/json; charset=utf-8");

print_info("API 2.1: Exchanging tempToken for JWT...");
    http_response_t *resp = http_get_with_custom_headers(url, headers);
    curl_slist_free_all(headers); 

    if (!resp) {
        print_error("Failed to connect to authentication server (API 2.1)");
        return -1;
    }
    
    // [新增日志] 打印响应体 (JWT)
    if (resp->data) {
        print_info_fmt("[DEBUG] API 2.1 Response Length (JWT): %zu bytes", resp->size);
        print_info_fmt("[DEBUG] API 2.1 Response (JWT): %.60s...", resp->data);
    }
    
    if (resp->status_code != 200) {
        print_error_fmt("tempToken exchange failed (HTTP %ld)", resp->status_code);
        http_response_free(resp);
        return -1;
    }

    if (resp->data && resp->size > 0) {
        // [修复] 检查缓冲区大小
        if (resp->size >= sizeof(user->jwt_token)) {
            print_error_fmt("[FATAL] JWT size (%zu) exceeds user_info_t buffer (%zu)!", 
                            resp->size, sizeof(user->jwt_token));
        }
        strncpy(user->jwt_token, resp->data, sizeof(user->jwt_token) - 1);
        print_success("API 2.1: JWT acquired.");
    } else {
        print_error("API 2.1: Received empty response.");
        http_response_free(resp);
        return -1;
    }
    
    http_response_free(resp);
    return 0;
}

int auth_get_access_token_from_jwt(user_info_t *user) {
    char url[256];
    snprintf(url, sizeof(url), "%s/authrouter/auth/api/jwToken/check", AUTH_BASE);

    // (来自 API 2.2 规范的 Header)
    struct curl_slist *headers = NULL;
    char jwt_header[2100];
    snprintf(jwt_header, sizeof(jwt_header), "jwttoken: %s", user->jwt_token);
    
    headers = curl_slist_append(headers, "user-agent: Dart/3.7 (dart:io)");
    headers = curl_slist_append(headers, "host: cn.devecostudio.huawei.com");
    headers = curl_slist_append(headers, "content-type: application/json; charset=utf-8");
    headers = curl_slist_append(headers, jwt_header);
    headers = curl_slist_append(headers, "refresh: false");

    print_info("API 2.2: Exchanging JWT for AccessToken...");
    http_response_t *resp = http_get_with_custom_headers(url, headers);
    curl_slist_free_all(headers); // 释放 headers
    
    if (!resp) {
        print_error("Failed to connect to authentication server (API 2.2)");
        return -1;
    }
    
    if (resp->status_code != 200) {
        print_error_fmt("JWT validation failed (HTTP %ld)", resp->status_code);
        http_response_free(resp);
        return -1;
    }
    
    // 解析 JSON
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse authentication response (API 2.2)");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *status_val = yyjson_obj_get(root, "status");
    int status = yyjson_get_bool(status_val);
    
    if (!status) {
        print_error("Invalid JWT (API 2.2 response status false)");
        yyjson_doc_free(doc);
        return -1;
    }
    
    // (来自 API 2.2 响应结构)
    yyjson_val *user_info = yyjson_obj_get(root, "userInfo");
    if (user_info) {
        const char *user_id = yyjson_get_str(yyjson_obj_get(user_info, "userId"));
        const char *access_token = yyjson_get_str(yyjson_obj_get(user_info, "accessToken"));
        const char *nick_name = yyjson_get_str(yyjson_obj_get(user_info, "nickName"));
        
        if (user_id) {
            strncpy(user->user_id, user_id, sizeof(user->user_id) - 1);
            strncpy(user->team_id, user_id, sizeof(user->team_id) - 1); // TeamID 默认为 UserID
        }
        if (access_token) {
            strncpy(user->access_token, access_token, sizeof(user->access_token) - 1);
        }
        if (nick_name) {
            strncpy(user->nickname, nick_name, sizeof(user->nickname) - 1);
        }
        
        user->real_name = yyjson_get_bool(yyjson_obj_get(user_info, "realName"));
        
        print_success("API 2.2: AccessToken acquired.");
        yyjson_doc_free(doc);
        return 0;
    } else {
        print_error("API 2.2: Response missing 'userInfo' object.");
        yyjson_doc_free(doc);
        return -1;
    }
}

int auth_init_oauth(user_info_t *user) {
    printf("\n");
    printf("%s╔════════════════════════════════════════════════════════════╗%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("%s║  Huawei Developer Account Authentication                  ║%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════════════════════╝%s\n\n", 
           COLOR_CYAN, COLOR_RESET);
    
    char code[33];
    generate_random_code(code, sizeof(code));
    int port = 3904;
    
    char auth_url[512];
    snprintf(auth_url, sizeof(auth_url),
             "%s/console/DevEcoIDE/apply?port=%d&appid=1007&code=%s",
             AUTH_BASE, port, code);
    
    // ... (打印指示信息，保持不变) ...
    
    // 启动服务器并自动获取 temp_token
    // (http_server_get_oauth_token 内部会调用 hdc-lite shell aa start)
    char *temp_token = http_server_get_oauth_token(port, auth_url);
    
    if (!temp_token) {
        print_error("Failed to get authentication token (temp_token)");
        // ... (打印提示信息) ...
        return -1;
    }
    
    // [修改] 执行 API 2.1 (tempToken -> JWT)
    if (auth_exchange_temp_for_jwt(temp_token, user) != 0) {
        free(temp_token);
        return -1;
    }
    free(temp_token); // temp_token 已使用完毕

    // [修改] 执行 API 2.2 (JWT -> AccessToken)
    if (auth_get_access_token_from_jwt(user) != 0) {
        print_error("Token validation failed (API 2.2)");
        return -1;
    }
    
    print_success("Token validated successfully!");

    return 0;
}

// int auth_get_user_info(user_info_t *user) {
//     char url[256];
//     snprintf(url, sizeof(url), "%s/devspaceapi/v1/userinfo", AUTH_BASE);
    
//     // [修复] 显式传入 1 (true)
//     http_response_t *resp = http_get_authed(url, user, 1);
    
//     if (!resp) {
//         print_warning("Failed to get detailed user info (nickname)");
//         return -1;
//     }
    
//     if (resp->status_code != 200) {
//         char warning_msg[128];
//         snprintf(warning_msg, sizeof(warning_msg),
//                  "Failed to get detailed user info (HTTP %ld)", resp->status_code);
//         print_warning(warning_msg);
//         http_response_free(resp);
//         return -1;
//     }
    
//     // 解析 JSON
//     yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
//     http_response_free(resp);
    
//     if (!doc) {
//         print_warning("Failed to parse user info");
//         return -1;
//     }
    
//     yyjson_val *root = yyjson_doc_get_root(doc);
//     yyjson_val *body = yyjson_obj_get(root, "body");
    
//     if (body) {
//         const char *nickname = yyjson_get_str(yyjson_obj_get(body, "nickname"));
//         if (nickname) {
//             strncpy(user->nickname, nickname, sizeof(user->nickname) - 1);
//         } else if (user->nickname[0] == '\0') { // 仅在昵称为空时回退
//             strncpy(user->nickname, user->user_id, sizeof(user->nickname) - 1);
//         }
//     }
    
//     yyjson_doc_free(doc);
//     return 0;
// }