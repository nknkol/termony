// [新文件: horpkg/src/config.h]

#ifndef HORPKG_CONFIG_H
#define HORPKG_CONFIG_H

#include <yyjson.h>

// 转发声明 user_info_t (来自 auth.h)
struct user_info_s;

// 认证信息 (子结构)
typedef struct {
    char user_id[64];
    char nickname[128];
    char jwt_token[2048];
    char access_token[1024];
    char team_id[64];
    int real_name;
} auth_config_t;

// 镜像信息 (子结构)
typedef struct {
    char url[512];
    char name[128];
} mirror_config_t;

// 核心配置结构体
typedef struct {
    auth_config_t auth;             // (替换 token.conf)
    char device_uuid[256];          // (替换 uuid.conf)
    char cert_id[64];               // (替换 cert_id.conf)
    mirror_config_t primary_mirror; // (替换 mirrors.json)
    char last_hdc_port[16];         // 记住上次成功连接的HDC端口
    
    // (未来扩展)
    char default_mode[32];
    int parallel_jobs;
    int manual_auth; // Manual authentication mode (no auto-browser)

} horpkg_config_t;

// (全局配置实例)
extern horpkg_config_t g_config;

// (函数声明)
// 获取主配置文件 (~/.horpkg/config.json) 的路径
char* config_get_primary_path(void);

// 加载/保存主配置文件
int config_load(void); // 从 config.json 加载到 g_config
int config_save(void); // 将 g_config 保存到 config.json

// 助手函数：在 user_info_t 和 g_config.auth 之间同步数据
void config_update_auth_from_user(const struct user_info_s *user);
void config_apply_auth_to_user(struct user_info_s *user);

#endif // HORPKG_CONFIG_H
