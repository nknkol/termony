// build-hnp/horpkg/src/http_server.h
#ifndef HORPKG_HTTP_SERVER_H
#define HORPKG_HTTP_SERVER_H

// 启动本地 HTTP 服务器接收 OAuth 回调
// 返回获取到的 token，失败返回 NULL
char* http_server_get_oauth_token(int port, const char *auth_url);

#endif // HORPKG_HTTP_SERVER_H