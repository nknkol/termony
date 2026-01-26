// build-hnp/horpkg/src/signing.h
#ifndef HORPKG_SIGNING_H
#define HORPKG_SIGNING_H

#include "auth.h"
#include <stddef.h>

// 证书信息
typedef struct {
    char id[64];
    char name[128];
    char object_id[256];
    char sha256[128];
} cert_info_t;

// Provision 信息
typedef struct {
    char id[64];
    char name[128];
    char file_path[256];
    char url[512];
} provision_info_t;

// 签名相关函数 (修改签名以传入 user)
int signing_generate_keystore(const char *keystore_path, const char *alias, const char *password);
int signing_generate_csr(const char *keystore_path,
                         const char *alias,
                         const char *password, 
                         const char *csr_output_path,
                         char *csr_out, size_t csr_size);
int signing_request_cert(const user_info_t *user, const char *csr, cert_info_t *cert);
int signing_download_cert(const char *object_id, const user_info_t *user, const char *output_path);
int signing_get_cert_list_and_find(const user_info_t *user, const char *cert_name, cert_info_t *cert_out);
int signing_delete_cert(const user_info_t *user, const char *cert_id);
// int signing_import_cert(const char *keystore_path, const char *alias, const char *password, const char *cert_path);

// Provision 相关函数 (修改签名以传入 user)
int signing_create_provision(const user_info_t *user, const cert_info_t *cert, const char **device_ids, int device_count, const char *bundle_name, char **perms, int perm_count, provision_info_t *provision);
int signing_download_provision(const user_info_t *user, const char *provision_url, const char *output_path);

/**
 * @brief [新] 确保指定 bundleName 的 provision 配置文件存在于本地。
 * 如果本地不存在，则会自动执行 API 4.1(获取设备), API 5.1(创建Profile), API 6.1(下载Profile)。
 *
 * @param user 已认证的用户。
 * @param bundle_name 应用的包名。
 * @param cert [输入] 用于签名的证书 (必须包含有效的 cert->id)。
 * @param perms [输入] 申请的权限列表 (可选, 可为NULL).
 * @param perm_count [输入] 权限数量.
 * @param profile_path_out [输出] 缓冲区，用于存放 .p7b 文件在本地的最终路径。
 * @param profile_path_size 缓冲区大小。
 * @return 0 成功 (文件已存在或已下载), -1 失败。
 */
int signing_ensure_provision_for_bundle(const user_info_t *user, const char *bundle_name, const cert_info_t *cert, char **perms, int perm_count, char *profile_path_out, size_t profile_path_size);


// 设备管理 (修改签名以传入 user)
int signing_get_device_list(const user_info_t *user, char ***device_ids_out, char ***device_names_out, int *count);

#endif // HORPKG_SIGNING_H