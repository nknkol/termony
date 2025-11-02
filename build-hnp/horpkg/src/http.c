// build-hnp/horpkg/src/http.c
#include "http.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

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

// GET 请求
http_response_t* http_get(const char *url, const char *token) {
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
    
    // 设置请求头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    if (token) {
        char cookie[512];
        snprintf(cookie, sizeof(cookie), "Cookie: hwid_account=%s", token);
        headers = curl_slist_append(headers, cookie);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Horpkg/1.0");
    
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        free(response->data);
        free(response);
        return NULL;
    }
    
    return response;
}

// POST 请求
http_response_t* http_post(const char *url, const char *token, const char *data, const char *content_type) {
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
    
    // 设置请求头
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    
    char content_header[128];
    snprintf(content_header, sizeof(content_header), "Content-Type: %s", content_type);
    headers = curl_slist_append(headers, content_header);
    
    if (token) {
        char cookie[512];
        snprintf(cookie, sizeof(cookie), "Cookie: hwid_account=%s", token);
        headers = curl_slist_append(headers, cookie);
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Horpkg/1.0");
    
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
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