#define _POSIX_C_SOURCE 200809L  // 启用 POSIX 扩展

#include "signing.h"
#include "http.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // 添加：提供 mkstemp, close, unlink
#include <yyjson.h>
#include <errno.h>

#define API_BASE "https://connect-api.cloud.huawei.com"

// 生成密钥库（使用 hapsigntool ）
int signing_generate_keystore(const char *keystore_path, const char *alias, const char *password) {
    char cmd[1024];
    
    snprintf(cmd, sizeof(cmd),
             "hapsigntool generate-keypair "
             "-keyAlias \"%s\" "
             "-keyAlg \"ECC\" -keySize \"NIST-P-256\" "
             "-keystoreFile \"%s\" -keystorePwd \"%s\" "
             "-keyPwd \"%s\" "
             "2>&1",
             alias, keystore_path, password, password);
    
    print_info_fmt("[DEBUG] Executing: %s", cmd);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        print_error("Failed to execute keytool");
        return -1;
    }
    
    char line[256];
    int has_error = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "error") || strstr(line, "Error")) {
            has_error = 1;
            fprintf(stderr, "%s", line);
        }
    }
    
    int status = pclose(fp);
    
    if (status != 0 || has_error) {
        print_error("Keystore generation failed");
        return -1;
    }
    
    return 0;
}

// 生成 CSR
int signing_generate_csr(const char *keystore_path, const char *alias, const char *password, char *csr_out, size_t csr_size) {
    char cmd[2048];
    
    char *temp_file_path = get_config_path("horpkg_csr_XXXXXX");
    if (!temp_file_path) {
        print_error("Failed to get config path for CSR temp file.");
        return -1;
    }

    int fd = mkstemp(temp_file_path);
    if (fd == -1) {
        print_error("Failed to create temp file for CSR");
        print_error_fmt("System error: %s (Path: %s)", strerror(errno), temp_file_path); 
        free(temp_file_path);
        return -1;
    }
    close(fd);
    
    const char *lib_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/lib";

    snprintf(cmd, sizeof(cmd),
             "LD_LIBRARY_PATH=\"%s\" " 
             "hapsigntool generate-csr "
             "-keyAlias \"%s\" -keyPwd \"%s\" "
             "-subject \"C=CN,O=YourOrg,OU=Mobile,CN=%s\" " 
             "-signAlg \"SHA256withECDSA\" "
             "-keystoreFile \"%s\" -keystorePwd \"%s\" "
             "-outFile \"%s\" "
             "2>&1",
             lib_path, alias, password, alias, keystore_path, password, temp_file_path);
    
    print_info_fmt("[DEBUG] Executing: %s", cmd);

    int status = system(cmd);
    if (status != 0) {
        print_error("CSR generation failed");
        unlink(temp_file_path);
        free(temp_file_path);
        return -1;
    }
    
    // 读取 CSR
    FILE *fp = fopen(temp_file_path, "r");
    if (!fp) {
        print_error("Failed to read CSR file");
        unlink(temp_file_path);
        free(temp_file_path);
        return -1;
    }
    
    size_t total_read = 0;
    size_t n;
    while ((n = fread(csr_out + total_read, 1, csr_size - total_read - 1, fp)) > 0) {
        total_read += n;
    }
    csr_out[total_read] = '\0';
    
    fclose(fp);
    unlink(temp_file_path);
    free(temp_file_path);
    
    return 0;
}

// 申请证书
int signing_request_cert(const user_info_t *user, const char *csr, cert_info_t *cert) {
    char url[256];
    snprintf(url, sizeof(url), "%s/api/cps/harmony-cert-manage/v1/cert/add", API_BASE);
    
    // URL 编码 CSR
    // 简化版：这里应该做完整的 URL 编码
    char *encoded_csr = strdup(csr);
    if (!encoded_csr) {
        return -1;
    }
    
    // 构建表单数据
    char *post_data = malloc(strlen(encoded_csr) + 512);
    if (!post_data) {
        free(encoded_csr);
        return -1;
    }
    
    char cert_name[128];
    snprintf(cert_name, sizeof(cert_name), "horpkg_auto_%s.cer", user->user_id);
    
    snprintf(post_data, strlen(encoded_csr) + 512,
             "certType=1&csr=%s&certName=%s",
             encoded_csr, cert_name);
    
    http_response_t *resp = http_post_authed(url, user, post_data, 
                                           "application/x-www-form-urlencoded");
    
    free(encoded_csr);
    free(post_data);
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to request certificate");
        http_response_free(resp);
        return -1;
    }
    
    // 解析响应
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse certificate response");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *ret = yyjson_obj_get(root, "ret");
    int code = yyjson_get_int(yyjson_obj_get(ret, "code"));
    
    if (code != 0) {
        print_error("Certificate request failed");
        yyjson_doc_free(doc);
        return -1;
    }
    
    // 提取证书信息
    yyjson_val *harmony_cert = yyjson_obj_get(root, "harmonyCert");
    if (harmony_cert) {
        const char *id = yyjson_get_str(yyjson_obj_get(harmony_cert, "id"));
        const char *name = yyjson_get_str(yyjson_obj_get(harmony_cert, "certName"));
        const char *object_id = yyjson_get_str(yyjson_obj_get(harmony_cert, "certObjectId"));
        
        if (id) strncpy(cert->id, id, sizeof(cert->id) - 1);
        if (name) strncpy(cert->name, name, sizeof(cert->name) - 1);
        if (object_id) strncpy(cert->object_id, object_id, sizeof(cert->object_id) - 1);
    }
    
    yyjson_doc_free(doc);
    return 0;
}

// 下载证书
int signing_download_cert(const char *object_id, const user_info_t *user, const char *output_path) {
    char url[256];
    snprintf(url, sizeof(url), "%s/api/amis/app-manage/v1/objects/url/reapply", API_BASE);
    
    // (注意: API 6.1 使用 application/x-www-form-urlencoded)
    char post_data[512];
    snprintf(post_data, sizeof(post_data), "sourceUrls=%s", object_id);
    
    // [修改] 使用 http_post_authed 并传入 user
    http_response_t *resp = http_post_authed(url, user, post_data, 
                                           "application/x-www-form-urlencoded");
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to get certificate download URL");
        http_response_free(resp);
        return -1;
    }
    
    // 解析下载 URL
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse download URL response");
        return -1;
    }
    
    // (注意: API 6.1 的响应结构与 python 示例不同，
    //  python 示例 (api_9) 显示 "urlsInfo" 而旧 C 代码显示 "objects"。
    //  我们遵循 API 6.1 文档，使用 "urlsInfo"。)
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *urls_info = yyjson_obj_get(root, "urlsInfo");
    
    if (!urls_info || !yyjson_is_arr(urls_info)) {
        print_error("No 'urlsInfo' array in response");
        yyjson_doc_free(doc);
        return -1;
    }
    
    yyjson_val *first_obj = yyjson_arr_get_first(urls_info);
    const char *download_url = yyjson_get_str(yyjson_obj_get(first_obj, "newUrl"));
    
    if (!download_url) {
        print_error("No 'newUrl' in response");
        yyjson_doc_free(doc);
        return -1;
    }
    
    // [修改] http_download_file 是独立的，不需要认证
    int result = http_download_file(download_url, output_path);
    
    yyjson_doc_free(doc);
    return result;
}

// 获取设备列表
int signing_get_device_list(const user_info_t *user, char ***device_ids_out, char ***device_names_out, int *count) {
    char url[256];
    snprintf(url, sizeof(url), 
             "%s/api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0", 
             API_BASE);

    http_response_t *resp = http_get_authed(url, user, 0);
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to get device list");
        http_response_free(resp);
        return -1;
    }
    
    // 解析设备列表
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse device list");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *list = yyjson_obj_get(root, "list");
    
    if (!list) {
        yyjson_doc_free(doc);
        *count = 0;
        return 0;
    }
    
    size_t arr_size = yyjson_arr_size(list);
    *count = (int)arr_size;
    
    if (arr_size == 0) {
        yyjson_doc_free(doc);
        return 0;
    }
    
    // 分配内存
    *device_ids_out = malloc(arr_size * sizeof(char*));
    *device_names_out = malloc(arr_size * sizeof(char*));
    
    size_t idx, max;
    yyjson_val *device;
    yyjson_arr_foreach(list, idx, max, device) {
        const char *id = yyjson_get_str(yyjson_obj_get(device, "id"));
        const char *name = yyjson_get_str(yyjson_obj_get(device, "deviceName"));
        
        (*device_ids_out)[idx] = id ? strdup(id) : strdup("");
        (*device_names_out)[idx] = name ? strdup(name) : strdup("Unknown");
    }
    
    yyjson_doc_free(doc);
    return 0;
}

// 创建 Provision 配置
int signing_create_provision(const user_info_t *user, const cert_info_t *cert, 
                            const char **device_ids, int device_count, 
                            const char *bundle_name, provision_info_t *provision) {
    char url[256];
    snprintf(url, sizeof(url), 
             "%s/api/cps/provision-manage/v1/ide/test/provision/add", 
             API_BASE);
    
    // 构建 JSON 请求
    char provision_name[256];
    snprintf(provision_name, sizeof(provision_name), 
             "horpkg-debug_%s", bundle_name);
    
    // 替换 '.' 为 '_'
    for (char *p = provision_name; *p; p++) {
        if (*p == '.') *p = '_';
    }
    
    // 构建设备列表 JSON
    char device_list_json[2048] = "[";
    for (int i = 0; i < device_count; i++) {
        char temp[128];
        snprintf(temp, sizeof(temp), "%s\"%s\"", i > 0 ? "," : "", device_ids[i]);
        strcat(device_list_json, temp);
    }
    strcat(device_list_json, "]");
    
    // 构建完整的 JSON
    char post_data[4096];
    snprintf(post_data, sizeof(post_data),
             "{"
             "\"provisionName\":\"%s\","
             "\"aclPermissionList\":[],"
             "\"deviceList\":%s,"
             "\"certList\":[\"%s\"],"
             "\"packageName\":\"%s\""
             "}",
             provision_name, device_list_json, cert->id, bundle_name);
    
    http_response_t *resp = http_post_authed(url, user, post_data, "application/json");
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to create provision");
        http_response_free(resp);
        return -1;
    }
    
    // 解析响应
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse provision response");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *ret = yyjson_obj_get(root, "ret");
    int code = yyjson_get_int(yyjson_obj_get(ret, "code"));
    
    if (code != 0) {
        print_error("Provision creation failed");
        yyjson_doc_free(doc);
        return -1;
    }
    
    // 获取下载 URL
    const char *download_url = yyjson_get_str(yyjson_obj_get(root, "provisionFileUrl"));
    if (download_url) {
        strncpy(provision->url, download_url, sizeof(provision->url) - 1);
        strncpy(provision->name, provision_name, sizeof(provision->name) - 1);
    }
    
    yyjson_doc_free(doc);
    return 0;
}

// 下载 Provision 文件
int signing_download_provision(const user_info_t *user, const char *provision_object_id, const char *output_path) {
    return signing_download_cert(provision_object_id, user, output_path);
}