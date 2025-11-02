// build-hnp/horpkg/src/http.h
#ifndef HORPKG_HTTP_H
#define HORPKG_HTTP_H

#include <curl/curl.h>

// HTTP 请求结果结构体
typedef struct {
    char *data;
    size_t size;
    long status_code;
} http_response_t;

// HTTP 请求函数
http_response_t* http_get(const char *url, const char *token);
http_response_t* http_post(const char *url, const char *token, const char *data, const char *content_type);
http_response_t* http_delete(const char *url, const char *token, const char *data);
void http_response_free(http_response_t *response);

// 文件下载
int http_download_file(const char *url, const char *output_path);

#endif // HORPKG_HTTP_H