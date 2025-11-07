#include "config.h"
#include "utils.h"  // (需要 get_config_path, print_info 等) [horpkg/src/utils.h]
#include "auth.h"   // (需要 user_info_t) [horpkg/src/auth.h]
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

// 全局实例定义
horpkg_config_t g_config = {0};

// (私有) 助手函数：安全地复制字符串
static void safe_strncpy(char *dest, const char *src, size_t n) {
    if (src) {
        strncpy(dest, src, n - 1);
        dest[n - 1] = '\0';
    } else {
        dest[0] = '\0';
    }
}

// (私有) 助手函数：从 yyjson 安全地获取字符串
static void json_get_string(yyjson_val *obj, const char *key, char *dest, size_t n) {
    if (obj && key && dest) {
        yyjson_val *val = yyjson_obj_get(obj, key);
        safe_strncpy(dest, yyjson_get_str(val), n);
    }
}

// (私有) 助手函数：从 yyjson 安全地获取整数
static int json_get_int(yyjson_val *obj, const char *key) {
    if (obj && key) {
        yyjson_val *val = yyjson_obj_get(obj, key);
        return yyjson_get_int(val);
    }
    return 0;
}

// (私有) 助手函数：从 yyjson 安全地获取布尔值 (int)
static int json_get_bool_as_int(yyjson_val *obj, const char *key) {
    if (obj && key) {
        yyjson_val *val = yyjson_obj_get(obj, key);
        return yyjson_get_bool(val) ? 1 : 0;
    }
    return 0;
}

// 获取主配置文件路径
char* config_get_primary_path(void) {
    return get_config_path("config.json"); // [horpkg/src/utils.c]
}

// 设置默认配置 (如果文件不存在)
static void config_set_defaults(void) {
    print_info("[Config] config.json not found or invalid. Loading defaults.");
    
    // (设置默认值)
    safe_strncpy(g_config.default_mode, "hnp", sizeof(g_config.default_mode));
    g_config.parallel_jobs = 4;
    
    // (设置默认镜像，替换 mirrors.json)
    safe_strncpy(g_config.primary_mirror.url, "https://raw.githubusercontent.com/nknkol/horpkg-index/main", sizeof(g_config.primary_mirror.url));
    safe_strncpy(g_config.primary_mirror.name, "GitHub (Default)", sizeof(g_config.primary_mirror.name));
    
    // 确保其他字段为空
    g_config.auth.user_id[0] = '\0';
    g_config.auth.jwt_token[0] = '\0';
    g_config.device_uuid[0] = '\0';
    g_config.cert_id[0] = '\0';
    g_config.last_hdc_port[0] = '\0';
}

// 从 config.json 加载配置
int config_load(void) {
    char *cfg_path = config_get_primary_path();
    if (!cfg_path) {
        return -1;
    }
    
    yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *doc = yyjson_read_file(cfg_path, flg, NULL, NULL);
    free(cfg_path);

    if (!doc) {
        // 配置文件不存在或无效
        config_set_defaults();
        return 0; // 返回成功，表示已加载（默认）配置
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    
    // (加载 Auth)
    yyjson_val *auth = yyjson_obj_get(root, "auth");
    json_get_string(auth, "user_id", g_config.auth.user_id, sizeof(g_config.auth.user_id));
    json_get_string(auth, "nickname", g_config.auth.nickname, sizeof(g_config.auth.nickname));
    json_get_string(auth, "jwt_token", g_config.auth.jwt_token, sizeof(g_config.auth.jwt_token));
    json_get_string(auth, "access_token", g_config.auth.access_token, sizeof(g_config.auth.access_token));
    json_get_string(auth, "team_id", g_config.auth.team_id, sizeof(g_config.auth.team_id));
    g_config.auth.real_name = json_get_bool_as_int(auth, "real_name");

    // (加载其他设置)
    json_get_string(root, "device_uuid", g_config.device_uuid, sizeof(g_config.device_uuid));
    json_get_string(root, "cert_id", g_config.cert_id, sizeof(g_config.cert_id));
    json_get_string(root, "last_hdc_port", g_config.last_hdc_port, sizeof(g_config.last_hdc_port));

    // (加载 Mirror)
    yyjson_val *mirror = yyjson_obj_get(root, "primary_mirror");
    json_get_string(mirror, "url", g_config.primary_mirror.url, sizeof(g_config.primary_mirror.url));
    json_get_string(mirror, "name", g_config.primary_mirror.name, sizeof(g_config.primary_mirror.name));

    // (加载未来扩展)
    json_get_string(root, "default_mode", g_config.default_mode, sizeof(g_config.default_mode));
    g_config.parallel_jobs = json_get_int(root, "parallel_jobs");

    // (确保默认值)
    if (g_config.primary_mirror.url[0] == '\0') {
        safe_strncpy(g_config.primary_mirror.url, "https://raw.githubusercontent.com/nknkol/horpkg-index/main", sizeof(g_config.primary_mirror.url));
    }
    if (g_config.default_mode[0] == '\0') {
        safe_strncpy(g_config.default_mode, "hnp", sizeof(g_config.default_mode));
    }
    if (g_config.parallel_jobs == 0) {
        g_config.parallel_jobs = 4;
    }
    
    yyjson_doc_free(doc);
    print_info("[Config] Loaded settings from config.json");
    return 0;
}

// 保存 g_config 到 config.json
int config_save(void) {
    char *cfg_path = config_get_primary_path();
    if (!cfg_path) {
        return -1;
    }

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    // (保存 Auth)
    yyjson_mut_val *auth = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, root, "auth", auth);
    yyjson_mut_obj_add_str(doc, auth, "user_id", g_config.auth.user_id);
    yyjson_mut_obj_add_str(doc, auth, "nickname", g_config.auth.nickname);
    yyjson_mut_obj_add_str(doc, auth, "jwt_token", g_config.auth.jwt_token);
    yyjson_mut_obj_add_str(doc, auth, "access_token", g_config.auth.access_token);
    yyjson_mut_obj_add_str(doc, auth, "team_id", g_config.auth.team_id);
    yyjson_mut_obj_add_bool(doc, auth, "real_name", g_config.auth.real_name);

    // (保存其他设置)
    yyjson_mut_obj_add_str(doc, root, "device_uuid", g_config.device_uuid);
    yyjson_mut_obj_add_str(doc, root, "cert_id", g_config.cert_id);
    yyjson_mut_obj_add_str(doc, root, "last_hdc_port", g_config.last_hdc_port);

    // (保存 Mirror)
    yyjson_mut_val *mirror = yyjson_mut_obj(doc);
    yyjson_mut_obj_add_val(doc, root, "primary_mirror", mirror);
    yyjson_mut_obj_add_str(doc, mirror, "url", g_config.primary_mirror.url);
    yyjson_mut_obj_add_str(doc, mirror, "name", g_config.primary_mirror.name);

    // (保存未来扩展)
    yyjson_mut_obj_add_str(doc, root, "default_mode", g_config.default_mode);
    yyjson_mut_obj_add_int(doc, root, "parallel_jobs", g_config.parallel_jobs);

    // (写入文件)
    yyjson_write_flag flg = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_SLASHES;
    if (yyjson_mut_write_file(cfg_path, doc, flg, NULL, NULL) == false) {
        print_error_fmt("Failed to write config.json to %s", cfg_path);
        yyjson_mut_doc_free(doc);
        free(cfg_path);
        return -1;
    }

    yyjson_mut_doc_free(doc);
    free(cfg_path);
    print_info("[Config] Settings saved to config.json");
    return 0;
}

// --- 助手函数 ---

void config_update_auth_from_user(const struct user_info_s *user) {
    if (!user) return;
    safe_strncpy(g_config.auth.user_id, user->user_id, sizeof(g_config.auth.user_id));
    safe_strncpy(g_config.auth.nickname, user->nickname, sizeof(g_config.auth.nickname));
    safe_strncpy(g_config.auth.jwt_token, user->jwt_token, sizeof(g_config.auth.jwt_token));
    safe_strncpy(g_config.auth.access_token, user->access_token, sizeof(g_config.auth.access_token));
    safe_strncpy(g_config.auth.team_id, user->team_id, sizeof(g_config.auth.team_id));
    g_config.auth.real_name = user->real_name;
}

void config_apply_auth_to_user(struct user_info_s *user) {
    if (!user) return;
    safe_strncpy(user->user_id, g_config.auth.user_id, sizeof(user->user_id));
    safe_strncpy(user->nickname, g_config.auth.nickname, sizeof(user->nickname));
    safe_strncpy(user->jwt_token, g_config.auth.jwt_token, sizeof(user->jwt_token));
    safe_strncpy(user->access_token, g_config.auth.access_token, sizeof(user->access_token));
    safe_strncpy(user->team_id, g_config.auth.team_id, sizeof(user->team_id));
    user->real_name = g_config.auth.real_name;
}
