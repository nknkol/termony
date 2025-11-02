#include "auth.h"
#include "http.h"
#include "http_server.h"  // ← 添加
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

static int open_browser_on_device(const char *url) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "hdc-lite shell aa start "
             "-A ohos.want.action.viewData "
             "-e entity.system.browsable "
             "-U \"%s\"",
             url);
    
    int ret = system(cmd);
    return (ret == 0) ? 0 : -1;
}

// OAuth 认证（在鸿蒙设备上打开浏览器）
int auth_init_oauth(user_info_t *user) {
    printf("\n");
    printf("%s╔════════════════════════════════════════════════════════════╗%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("%s║  Huawei Developer Account Authentication                  ║%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════════════════════╝%s\n\n", 
           COLOR_CYAN, COLOR_RESET);
    
    // 生成随机授权码
    char code[33];
    generate_random_code(code, sizeof(code));
    
    // 本地服务器端口
    int port = 3904;
    
    // 构建授权 URL
    char auth_url[512];
    snprintf(auth_url, sizeof(auth_url),
             "%s/console/DevEcoIDE/apply?port=%d&appid=1007&code=%s",
             AUTH_BASE, port, code);
    
    print_info("Starting local OAuth callback server...");
    printf("\n%sAuthentication URL:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("%s%s%s\n\n", COLOR_CYAN, auth_url, COLOR_RESET);
    
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n", 
           COLOR_YELLOW, COLOR_RESET);
    printf("%sℹ️  Instructions:%s\n\n", COLOR_YELLOW, COLOR_RESET);
    printf("  1. A browser window will open on your device\n");
    printf("  2. Login with your Huawei Developer Account\n");
    printf("  3. Token will be captured automatically\n");
    printf("  4. Do NOT close this terminal window!\n");
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━%s\n\n", 
           COLOR_YELLOW, COLOR_RESET);
    
    // 启动服务器并自动获取 token
    char *token = http_server_get_oauth_token(port, auth_url);
    
    if (!token) {
        print_error("Failed to get authentication token");
        print_prompt("Please ensure:");
        printf("  • You completed the login process\n");
        printf("  • Your network connection is stable\n");
        printf("  • Port %d is not blocked\n", port);
        return -1;
    }
    
    // 保存 token
    strncpy(user->token, token, sizeof(user->token) - 1);
    free(token);
    
    printf("\n");
    print_info("Validating token...");
    
    // 验证 Token
    if (auth_check_jwt_token(user->token, user) != 0) {
        print_error("Token validation failed");
        return -1;
    }
    
    print_success("Token validated successfully!");
    
    return 0;
}

// 检查 JWT Token
int auth_check_jwt_token(const char *token, user_info_t *user) {
    char url[256];
    snprintf(url, sizeof(url), "%s/authrouter/auth/api/jwToken/check", AUTH_BASE);
    
    http_response_t *resp = http_get(url, token);
    if (!resp) {
        print_error("Failed to connect to authentication server");
        return -1;
    }
    
    if (resp->status_code != 200) {
        print_error_fmt("Token validation failed (HTTP %ld)", resp->status_code);
        http_response_free(resp);
        return -1;
    }
    
    // 解析 JSON
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse authentication response");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *status_val = yyjson_obj_get(root, "status");
    int status = yyjson_get_bool(status_val);
    
    if (!status) {
        print_error("Invalid token");
        yyjson_doc_free(doc);
        return -1;
    }
    
    // 提取用户信息
    yyjson_val *user_info = yyjson_obj_get(root, "userInfo");
    if (user_info) {
        const char *user_id = yyjson_get_str(yyjson_obj_get(user_info, "userId"));
        if (user_id) {
            strncpy(user->user_id, user_id, sizeof(user->user_id) - 1);
        }
        
        user->real_name = yyjson_get_bool(yyjson_obj_get(user_info, "realName"));
    }
    
    yyjson_doc_free(doc);
    return 0;
}

// 获取用户详细信息
int auth_get_user_info(const char *token, user_info_t *user) {
    char url[256];
    snprintf(url, sizeof(url), "%s/devspaceapi/v1/userinfo", AUTH_BASE);
    
    http_response_t *resp = http_get(url, token);
    if (!resp) {
        print_warning("Failed to get detailed user info");
        return -1;
    }
    
    if (resp->status_code != 200) {
        // ← 修复：先格式化字符串
        char warning_msg[128];
        snprintf(warning_msg, sizeof(warning_msg),
                 "Failed to get detailed user info (HTTP %ld)", resp->status_code);
        print_warning(warning_msg);
        http_response_free(resp);
        return -1;
    }
    
    // 解析 JSON
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_warning("Failed to parse user info");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *body = yyjson_obj_get(root, "body");
    
    if (body) {
        const char *nickname = yyjson_get_str(yyjson_obj_get(body, "nickname"));
        if (nickname) {
            strncpy(user->nickname, nickname, sizeof(user->nickname) - 1);
        } else {
            // 如果没有昵称，使用用户 ID
            strncpy(user->nickname, user->user_id, sizeof(user->nickname) - 1);
        }
    }
    
    yyjson_doc_free(doc);
    return 0;
}