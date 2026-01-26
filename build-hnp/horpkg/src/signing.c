#define _POSIX_C_SOURCE 200809L

#include "signing.h"
#include "http.h"
#include "utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // 添加：提供 mkstemp, close, unlink, access
#include <yyjson.h>
#include <errno.h>
#include <time.h>

#define API_BASE "https://connect-api.cloud.huawei.com"

// 生成密钥库（使用 hapsigntool ）
int signing_generate_keystore(const char *keystore_path, const char *alias, const char *password) {
    char cmd[1024];
    
    snprintf(cmd, sizeof(cmd),
             "hapsigntool generate-keypair "
             "-keyAlias \"%s\" "
             "-keyAlg \"ECC\" -keySize \"NIST-P-384\" "
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

/**
 * @brief (修改) 生成 CSR 并将其保存到指定路径
 */
int signing_generate_csr(const char *keystore_path, const char *alias, const char *password, 
                         const char *csr_output_path, // <-- [新增]
                         char *csr_out, size_t csr_size) {
    
    char cmd[2048];
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
             lib_path, alias, password, alias, keystore_path, password, csr_output_path);
    
    print_info_fmt("[DEBUG] Executing: %s", cmd);

    int status = system(cmd);
    if (status != 0) {
        print_error("CSR generation failed");
        return -1;
    }
    
    // 读取 CSR
    FILE *fp = fopen(csr_output_path, "r");
    if (!fp) {
        print_error("Failed to read CSR file");
        return -1;
    }
    
    size_t total_read = 0;
    size_t n;
    while ((n = fread(csr_out + total_read, 1, csr_size - total_read - 1, fp)) > 0) {
        total_read += n;
    }
    csr_out[total_read] = '\0';
    
    fclose(fp);

    return 0;
}

/**
 * @brief 调用 API 3 (获取证书列表) 并查找特定名称的有效证书
 * @param user 已认证的用户
 * @param cert_name 要查找的证书名称 (例如 "horpkg")
 * @param cert_out [输出] 用于填充找到的证书信息的结构体
 * @return 0 表示成功找到, -1 表示未找到或出错
 */
int signing_get_cert_list_and_find(const user_info_t *user, const char *cert_name, cert_info_t *cert_out) {
    char url[256];
    snprintf(url, sizeof(url), "%s/api/cps/harmony-cert-manage/v1/cert/list", API_BASE);
    
    print_info("API 3: Checking cloud certificate list...");
    
    // API 3 (Get Cert List) 使用 POST 和 form-urlencoded，但请求体为空
    // (参考 DevEco_Login_API_Documentation.md)
    http_response_t *resp = http_post_authed(url, user, NULL, 
                                           "application/x-www-form-urlencoded; charset=UTF-8");
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to get certificate list (API 3)");
        http_response_free(resp);
        return -1;
    }

    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);

    if (!doc) {
        print_error("Failed to parse certificate list response (API 3)");
        return -1;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *ret = yyjson_obj_get(root, "ret");
    if (!ret || yyjson_get_int(yyjson_obj_get(ret, "code")) != 0) {
        print_error_fmt("Certificate list API returned an error: %s", 
                      yyjson_get_str(yyjson_obj_get(ret, "msg")));
        yyjson_doc_free(doc);
        return -1;
    }

    yyjson_val *cert_list = yyjson_obj_get(root, "certList");
    if (!cert_list || !yyjson_is_arr(cert_list)) {
        print_warning("No 'certList' array in response. (No certificates on cloud)");
        yyjson_doc_free(doc);
        return -1; // 没有列表
    }

    size_t idx, max;
    yyjson_val *cert_json;
    // 获取当前时间的毫秒数
    long long current_time_ms = (long long)time(NULL) * 1000;

    yyjson_arr_foreach(cert_list, idx, max, cert_json) {
        const char *name = yyjson_get_str(yyjson_obj_get(cert_json, "certName"));
        
        if (name && strcmp(name, cert_name) == 0) {
            // 找到了同名证书，检查其有效性
            int status = yyjson_get_int(yyjson_obj_get(cert_json, "status"));
            long long expire_time_ms = yyjson_get_uint(yyjson_obj_get(cert_json, "expireTime"));

            if (status == 1 && expire_time_ms > current_time_ms) {
                // 找到了一个有效的、未过期的证书！
                print_info("Found valid, non-expired certificate on cloud.");
                
                const char *id = yyjson_get_str(yyjson_obj_get(cert_json, "id"));
                const char *object_id = yyjson_get_str(yyjson_obj_get(cert_json, "certObjectId"));
                
                if (id) strncpy(cert_out->id, id, sizeof(cert_out->id) - 1);
                if (name) strncpy(cert_out->name, name, sizeof(cert_out->name) - 1);
                if (object_id) strncpy(cert_out->object_id, object_id, sizeof(cert_out->object_id) - 1);
                
                yyjson_doc_free(doc);
                return 0; // 成功找到！
            } else {
                print_warning_fmt("Found matching cert '%s', but it is invalid (Status: %d) or expired.", name, status);
            }
        }
    }

    // 循环结束，未找到匹配的有效证书
    print_info("No valid certificate matching 'horpkg' found on cloud.");
    yyjson_doc_free(doc);
    return -1; // 未找到
}

/**
 * @brief 删除证书 (API 3.2)
 * @param user 已认证的用户
 * @param cert_id 要删除的证书ID
 * @return 0 成功, -1 失败
 */
int signing_delete_cert(const user_info_t *user, const char *cert_id) {
    char url[256];
    snprintf(url, sizeof(url), "%s/api/cps/harmony-cert-manage/v1/cert/delete", API_BASE);
    
    // 构建 JSON 请求体 (API 3.2 需要 JSON)
    char post_data[256];
    snprintf(post_data, sizeof(post_data),
             "{\"certIds\":[\"%s\"]}",
             cert_id);
    
    print_info_fmt("API 4: Deleting certificate (ID: %s)...", cert_id);
    
    // 调用我们新创建的 http_delete_authed 函数
    http_response_t *resp = http_delete_authed(url, user, post_data, "application/json");
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to delete certificate (HTTP error)");
        http_response_free(resp);
        return -1;
    }
    
    // 解析响应
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        print_error("Failed to parse delete certificate response");
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *ret = yyjson_obj_get(root, "ret");
    int code = yyjson_get_int(yyjson_obj_get(ret, "code"));
    
    if (code != 0) {
        print_error_fmt("Certificate deletion failed (API code %d): %s", 
                      code, yyjson_get_str(yyjson_obj_get(ret, "msg")));
        yyjson_doc_free(doc);
        return -1;
    }
    
    print_success("Certificate deleted successfully.");
    yyjson_doc_free(doc);
    return 0;
}

// 申请证书
/*
 * 位于: horpkg/src/signing.c
 *
 * (确保在此文件顶部已 #include "logger.h" 和 <curl/curl.h> (通常通过 http.h 间接包含)
 */
// 申请证书
int signing_request_cert(const user_info_t *user, const char *csr, cert_info_t *cert) {
    char url[256];
    snprintf(url, sizeof(url), "%s/api/cps/harmony-cert-manage/v1/cert/add", API_BASE);
    
    // --- [修复开始] ---
    // `csr` 变量包含完整的 CSR 内容 (Key A, 包含页眉和 \n 换行符)
    
    // 1. 使用 curl_easy_escape 进行 URL 编码
    //    这将把 \n 转换为 %0A, + 转换 %2B, 空格 转换为 + (或 %20)
    char *encoded_csr = curl_easy_escape(NULL, csr, 0); 
    if (!encoded_csr) {
        log_error("Failed to URL-encode CSR string."); // [!] 使用 logger
        return -1;
    }
    
    char cert_name[128];
    snprintf(cert_name, sizeof(cert_name), "horpkg_auto_%s.cer", user->user_id);

    // 2. 计算 post_data 的正确缓冲区大小 (基于 *已编码* 的 CSR)
    size_t post_data_len = strlen("certType=1&csr=") + strlen(encoded_csr) + 
                           strlen("&certName=") + strlen(cert_name) + 1;
    
    char *post_data = malloc(post_data_len);
    if (!post_data) {
        curl_free(encoded_csr); 
        log_error("Failed to allocate memory for post data."); // [!] 使用 logger
        return -1;
    }
    
    // 3. 使用正确的长度安全地构建 post_data
    snprintf(post_data, post_data_len,
             "certType=1&csr=%s&certName=%s",
             encoded_csr, cert_name);
    
    // 4. [!] 释放 curl_easy_escape 的内存 (post_data 还需要)
    curl_free(encoded_csr);
    
    // --- [修复结束] ---

    // --- [新增日志：按照您的要求] ---
    char *log_path = get_config_path("log.txt");
    if (log_path) {
        FILE *f_log = fopen(log_path, "a");
        if (f_log) {
            time_t t = time(NULL);
            char time_buf[100];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
            
            fprintf(f_log, "--- BEGIN POST (API 5) [%s] ---\n", time_buf);
            fprintf(f_log, "URL: %s\n", url);
            fprintf(f_log, "POST_BODY: %s\n", post_data); // 记录 *已编码* 的数据
            fprintf(f_log, "--- END POST ---\n\n");
            fclose(f_log);
            log_info("Debug POST data saved to: %s", log_path);
        }
        free(log_path);
    }
    // --- [新增日志结束] ---
    
    http_response_t *resp = http_post_authed(url, user, post_data, 
                                           "application/x-www-form-urlencoded");
    
    // 5. [!] 释放 post_data 的内存
    free(post_data);
    
    if (!resp || resp->status_code != 200) {
        log_error("Failed to request certificate (HTTP %ld)", resp ? resp->status_code : 0); // [!] 使用 logger
        http_response_free(resp);
        return -1;
    }
    
    // (其余的 JSON 解析逻辑保持不变)
    yyjson_doc *doc = yyjson_read(resp->data, resp->size, 0);
    http_response_free(resp);
    
    if (!doc) {
        log_error("Failed to parse certificate response JSON"); // [!] 使用 logger
        return -1;
    }
    
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *ret = yyjson_obj_get(root, "ret");
    int code = yyjson_get_int(yyjson_obj_get(ret, "code"));
    
    if (code != 0) {
        log_error("Certificate request failed (API code %d): %s", 
                  code, yyjson_get_str(yyjson_obj_get(ret, "msg"))); // [!] 使用 logger
        yyjson_doc_free(doc);
        return -1;
    }
    
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
    // [!] download_file 位于 download.c
    int download_file(const char *url, const char *outfile);
    int result = download_file(download_url, output_path);
    
    yyjson_doc_free(doc);
    return result;
}

// 获取设备列表
int signing_get_device_list(const user_info_t *user, char ***device_ids_out, char ***device_names_out, int *count) {
    char url[256];
    snprintf(url, sizeof(url), 
             "%s/api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0", 
             API_BASE);

    // [!] API 4.1 (get_device_list) 不发送 content-type，但需要 accept: application/json (http_get_authed 默认)
    // [!] 修正：根据 API 文档，它不需要 content-type，也不需要 accept:json。
    // 我们使用 http_get_authed(..., user, 0) 来发送认证头，但不发送 accept:json
    http_response_t *resp = http_get_authed(url, user, 0); 
    
    if (!resp || resp->status_code != 200) {
        print_error("Failed to get device list");
        if (resp) {
            print_error_fmt("HDC Response (Data): %s", resp->data ? resp->data : "NULL");
        }
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
        *device_ids_out = NULL;
        *device_names_out = NULL;
        return 0;
    }
    
    size_t arr_size = yyjson_arr_size(list);
    *count = (int)arr_size;
    
    if (arr_size == 0) {
        yyjson_doc_free(doc);
        *device_ids_out = NULL;
        *device_names_out = NULL;
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
                            const char *bundle_name, char **perms, int perm_count, provision_info_t *provision) {
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
    
    // 构建权限列表 JSON
    // 注意：需要足够大的缓冲区，每个权限名约 50 字节，如果有 10 个权限，大约 500 字节
    char acl_list_json[4096] = "[]";
    if (perms && perm_count > 0) {
        strcpy(acl_list_json, "[");
        for (int i = 0; i < perm_count; i++) {
            char temp[128];
            snprintf(temp, sizeof(temp), "%s\"%s\"", i > 0 ? "," : "", perms[i]);
            // 简单检查防止溢出
            if (strlen(acl_list_json) + strlen(temp) < sizeof(acl_list_json) - 1) {
                strcat(acl_list_json, temp);
            }
        }
        strcat(acl_list_json, "]");
    }

    // 构建完整的 JSON
    char post_data[8192]; // 增加缓冲区大小
    snprintf(post_data, sizeof(post_data),
             "{"
             "\"provisionName\":\"%s\","
             "\"aclPermissionList\":%s," // 插入 acl_list_json
             "\"deviceList\":%s,"
             "\"certList\":[\"%s\"],"
             "\"packageName\":\"%s\""
             "}",
             provision_name, acl_list_json, device_list_json, cert->id, bundle_name);
    
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
    // 下载 provision 和下载 cert 使用完全相同的 API (6.1)
    return signing_download_cert(provision_object_id, user, output_path);
}

// [新] 确保 Provision 文件存在
int signing_ensure_provision_for_bundle(const user_info_t *user, const char *bundle_name, const cert_info_t *cert, char **perms, int perm_count, char *profile_path_out, size_t profile_path_size) {
    
    char profile_filename[256];
    snprintf(profile_filename, sizeof(profile_filename), "%s.p7b", bundle_name);

    char *local_path = get_provision_path(profile_filename);
    if (!local_path) {
        log_error("Failed to construct local profile path.");
        return -1;
    }

    // 1. 检查本地是否已存在
    if (access(local_path, F_OK) == 0) {
        print_info_fmt("Found existing local profile: %s", local_path);
        strncpy(profile_path_out, local_path, profile_path_size - 1);
        free(local_path);
        return 0;
    }

    log_warn("Local profile '%s' not found. Requesting from cloud...", profile_filename);

    // 2. 获取设备列表 (API 4.1)
    char **device_ids = NULL;
    char **device_names = NULL;
    int device_count = 0;
    
    if (signing_get_device_list(user, &device_ids, &device_names, &device_count) != 0) {
        log_error("Failed to get device list (API 4.1). Cannot create profile.");
        free(local_path);
        return -1;
    }

    if (device_count == 0) {
        log_error("No devices registered to your account. Cannot create profile.");
        print_prompt("Please register your device in DevEco Studio or developer.huawei.com");
        free(local_path);
        return -1;
    }

    log_info("Using %d registered device(s) for profile.", device_count);

    // 3. 创建 Provision (API 5.1)
    provision_info_t provision = {0};
    if (signing_create_provision(user, cert, (const char**)device_ids, device_count, bundle_name, perms, perm_count, &provision) != 0) {
        print_error_fmt("Failed to create provision profile for '%s' (API 5.1).", bundle_name);
        // (清理)
        for (int i = 0; i < device_count; i++) { free(device_ids[i]); free(device_names[i]); }
        free(device_ids); free(device_names);
        free(local_path);
        return -1;
    }

    print_success_fmt("Successfully created profile '%s' on cloud.", provision.name);

    // 4. 下载 Provision (API 6.1)
    if (signing_download_provision(user, provision.url, local_path) != 0) {
        print_error_fmt("Failed to download profile to '%s' (API 6.1).", local_path);
        // (清理)
        for (int i = 0; i < device_count; i++) { free(device_ids[i]); free(device_names[i]); }
        free(device_ids); free(device_names);
        free(local_path);
        return -1;
    }

    print_success_fmt("Profile downloaded successfully: %s", local_path);
    strncpy(profile_path_out, local_path, profile_path_size - 1);

    // (最终清理)
    for (int i = 0; i < device_count; i++) { free(device_ids[i]); free(device_names[i]); }
    free(device_ids); free(device_names);
    free(local_path);
    
    return 0;
}
