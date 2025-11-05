#ifndef HORPKG_HTTP_H
#define HORPKG_HTTP_H

#include <curl/curl.h>

typedef struct {
    char *data;
    size_t size;
    long status_code;
} http_response_t;

struct user_info_s;

http_response_t* http_get_with_custom_headers(const char *url, struct curl_slist *headers);
http_response_t* http_get_authed(const char *url, const struct user_info_s *user, int send_accept_json);
http_response_t* http_delete_authed(const char *url, const struct user_info_s *user, const char *data, const char *content_type);
http_response_t* http_post_authed(const char *url, const struct user_info_s *user, const char *data, const char *content_type);

void http_response_free(http_response_t *response);
int http_download_file(const char *url, const char *output_path);

#endif // HORPKG_HTTP_H