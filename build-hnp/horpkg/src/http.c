#include "http.h"
#include "utils.h"
#include "auth.h"
#include <stdlib.h>
#include <string.h>
#include "logger.h"

// libcurl 回调函数
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    http_response_t *mem = (http_response_t *)userp;
    
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        print_error("Memory allocation failed");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    
    return realsize;
}
// 辅助函数：构建标准的认证 Header
static struct curl_slist* build_authed_headers(const struct user_info_s *user, const char* content_type, int send_accept_json) {
    struct curl_slist *headers = NULL;
    char uid_header[128];
    char token_header[2100]; // (保持 2.1k 缓冲区)
    char team_header[128];

    // 检查 access_token 是否有效
    if (!user || user->access_token[0] == '\0') {
        log_error("[HTTP] build_authed_headers: access_token is missing.");
        return NULL;
    }

    snprintf(uid_header, sizeof(uid_header), "uid: %s", user->user_id);
    snprintf(token_header, sizeof(token_header), "oauth2token: %s", user->access_token);
    snprintf(team_header, sizeof(team_header), "teamid: %s", user->team_id);
    
    headers = curl_slist_append(headers, "user-agent: Dart/3.7 (dart:io)");
    // headers = curl_slist_append(headers, "accept-encoding: gzip");
    headers = curl_slist_append(headers, "Host: connect-api.cloud.huawei.com");
    headers = curl_slist_append(headers, uid_header);
    headers = curl_slist_append(headers, token_header);
    headers = curl_slist_append(headers, team_header);
    
    // 如果是 POST，添加 Content-Type
    if (content_type) {
        char content_header[128];
        snprintf(content_header, sizeof(content_header), "Content-Type: %s", content_type);
        headers = curl_slist_append(headers, content_header);
    }
    
    if (send_accept_json) {
        headers = curl_slist_append(headers, "Accept: application/json");
    }
    
    return headers;
}

// --- [新增] 用于认证流程 (API 2.1, 2.2) ---
// (此函数不自动添加认证头，依赖传入的 headers)
http_response_t* http_get_with_custom_headers(const char *url, struct curl_slist *headers) {
    CURL *curl;
    CURLcode res;
    
    http_response_t *response = malloc(sizeof(http_response_t));
    response->data = malloc(1);
    response->size = 0;
    response->status_code = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        free(response->data);
        free(response);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // 使用传入的头
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Dart/3.7 (dart:io)");
    const char* ca_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/etc/cacert.pem";
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    

    log_debug("=== HTTP Response Body (URL: %s) ===", url);
    if (response->data && response->size > 0) {
        log_debug("%s", response->data);
    } else {
        log_debug("(No data received)");
    }
    log_debug("=== End of Response (HTTP %ld) ===", response->status_code);


    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        log_error("[HTTP] http_get_with_custom_headers failed: %s", curl_easy_strerror(res));
        free(response->data);
        free(response);
        return NULL;
    }
    
    return response;
}

http_response_t* http_get_authed(const char *url, const struct user_info_s *user, int send_accept_json) {
    CURL *curl;
    CURLcode res;
    
    http_response_t *response = malloc(sizeof(http_response_t));
    response->data = malloc(1);
    response->size = 0;
    response->status_code = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        free(response->data);
        free(response);
        return NULL;
    }

    struct curl_slist *headers = build_authed_headers(user, NULL, send_accept_json); 
    if (!headers) {
        curl_easy_cleanup(curl);
        free(response->data);
        free(response);
        return NULL; 
    }
    
    if (send_accept_json == 0) {
        headers = curl_slist_append(headers, "Accept:");
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);

    const char* ca_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/etc/cacert.pem";
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    
    log_debug("=== HTTP Response Body (URL: %s) ===", url);
    if (response->data && response->size > 0) {
        log_debug("%s", response->data);
    } else {
        log_debug("(No data received)");
    }
    log_debug("=== End of Response (HTTP %ld) ===", response->status_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        log_error("[HTTP] http_get_authed failed: %s", curl_easy_strerror(res));
        free(response->data);
        free(response);
        return NULL;
    }
    
    return response;
}

/**
 * @brief 发送一个带认证的 HTTP DELETE 请求
 */
http_response_t* http_delete_authed(const char *url, const struct user_info_s *user, const char *data, const char *content_type) {
    CURL *curl;
    CURLcode res;
    
    http_response_t *response = malloc(sizeof(http_response_t));
    response->data = malloc(1);
    response->size = 0;
    response->status_code = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        free(response->data);
        free(response);
        return NULL;
    }
    
    // (复用 build_authed_headers 来构建认证头)
    struct curl_slist *headers = build_authed_headers(user, content_type, 1);
    if (!headers) {
        curl_easy_cleanup(curl);
        free(response->data);
        free(response);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    // --- [关键] 设置请求方法为 DELETE ---
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    if (data) {
        // (DELETE 请求也可以携带 body)
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    }
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
    
    // (设置 CA 路径)
    const char* ca_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/etc/cacert.pem";
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    
    log_debug("=== HTTP Response Body (DELETE: %s) ===", url);
    if (response->data && response->size > 0) {
        log_debug("%s", response->data);
    } else {
        log_debug("(No data received)");
    }
    log_debug("=== End of Response (HTTP %ld) ===", response->status_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        log_error("[HTTP] http_delete_authed failed: %s", curl_easy_strerror(res));
        free(response->data);
        free(response);
        return NULL;
    }
    
    return response;
}

http_response_t* http_post_authed(const char *url, const struct user_info_s *user, const char *data, const char *content_type) {
    CURL *curl;
    CURLcode res;
    
    http_response_t *response = malloc(sizeof(http_response_t));
    response->data = malloc(1);
    response->size = 0;
    response->status_code = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        free(response->data);
        free(response);
        return NULL;
    }
    
    struct curl_slist *headers = build_authed_headers(user, content_type, 1);
    if (!headers) {
        curl_easy_cleanup(curl);
        free(response->data);
        free(response);
        return NULL;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    // curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    if (data) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
    
    const char* ca_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/etc/cacert.pem";
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    

    log_debug("=== HTTP Response Body (URL: %s) ===", url);
    if (response->data && response->size > 0) {
        log_debug("%s", response->data);
    } else {
        log_debug("(No data received)");
    }
    log_debug("=== End of Response (HTTP %ld) ===", response->status_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        log_error("[HTTP] http_post_authed failed: %s", curl_easy_strerror(res));
        free(response->data);
        free(response);
        return NULL;
    }
    
    return response;
}

// 释放响应
void http_response_free(http_response_t *response) {
    if (response) {
        if (response->data) {
            free(response->data);
        }
        free(response);
    }
}

// 下载文件
int http_download_file(const char *url, const char *output_path) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    
    curl = curl_easy_init();
    if (!curl) {
        return -1;
    }
    
    fp = fopen(output_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    res = curl_easy_perform(curl);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK) ? 0 : -1;
}