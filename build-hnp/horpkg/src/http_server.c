#define _DEFAULT_SOURCE
#include "http_server.h"
#include "utils.h"
#include "config.h" // [!] Added for g_config
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>      // ← 添加：提供 struct timeval
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>          // ← 添加：提供 time()

#define BUFFER_SIZE 8192
#define TIMEOUT_SECONDS 300  // 5分钟超时

// 全局变量存储接收到的 token
static char *g_received_token = NULL;
static int g_server_should_stop = 0;

// 从 HTTP 请求中提取参数值
static char* extract_param(const char *request, const char *param_name) {
    char search_str[128];
    snprintf(search_str, sizeof(search_str), "%s=", param_name);
    
    const char *start = strstr(request, search_str);
    if (!start) {
        return NULL;
    }
    
    start += strlen(search_str);
    const char *end = strstr(start, "&");
    if (!end) {
        end = strstr(start, " ");
    }
    if (!end) {
        end = start + strlen(start);
    }
    
    size_t len = end - start;
    char *value = malloc(len + 1);
    if (value) {
        strncpy(value, start, len);
        value[len] = '\0';
    }
    
    return value;
}

// HTTP 响应页面
static const char *SUCCESS_PAGE = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta charset='utf-8'>"
    "<title>Horpkg Authentication</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background: #f5f5f5; }"
    ".success { background: #4CAF50; color: white; padding: 20px; border-radius: 5px; display: inline-block; }"
    ".token { background: #fff; padding: 15px; margin: 20px; border: 1px solid #ddd; border-radius: 3px; word-break: break-all; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class='success'>"
    "<h1>✓ Authentication Successful!</h1>"
    "<p>You can close this window and return to the terminal.</p>"
    "</div>"
    "<div class='token'>"
    "<p><strong>Token has been automatically captured</strong></p>"
    "<p>Horpkg initialization will continue in the terminal...</p>"
    "</div>"
    "</body>"
    "</html>";

// 处理 HTTP 请求
static void handle_request(int client_sock) {
    char buffer[BUFFER_SIZE];
    ssize_t received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    
    if (received > 0) {
        buffer[received] = '\0';
        
        // 打印请求（调试用）
        printf("\n[DEBUG] Received HTTP request headers:\n");
        printf("═══════════════════════════════════════\n");
        printf("%s", buffer);
        printf("═══════════════════════════════════════\n");
        
        char *token = NULL;
        
        // 检查是否是 POST 请求
        if (strstr(buffer, "POST") == buffer) {
            // 找到 Content-Length
            const char *content_length_str = strstr(buffer, "Content-Length:");
            if (content_length_str) {
                int content_length = 0;
                sscanf(content_length_str, "Content-Length: %d", &content_length);
                
                if (content_length > 0 && content_length < BUFFER_SIZE) {
                    // 找到请求头结束位置（\r\n\r\n）
                    char *body_start = strstr(buffer, "\r\n\r\n");
                    if (body_start) {
                        body_start += 4;  // 跳过 \r\n\r\n
                        
                        // 计算已经接收的 body 长度
                        int body_received = received - (body_start - buffer);
                        
                        // 如果还有更多数据需要读取
                        if (body_received < content_length) {
                            int remaining = content_length - body_received;
                            char *body_buffer = malloc(content_length + 1);
                            
                            if (body_buffer) {
                                // 复制已接收的部分
                                memcpy(body_buffer, body_start, body_received);
                                
                                // 接收剩余的数据
                                ssize_t extra = recv(client_sock, 
                                                    body_buffer + body_received, 
                                                    remaining, 0);
                                
                                if (extra > 0) {
                                    body_received += extra;
                                }
                                
                                body_buffer[body_received] = '\0';
                                
                                printf("\n[DEBUG] POST Body:\n");
                                printf("═══════════════════════════════════════\n");
                                printf("%s\n", body_buffer);
                                printf("═══════════════════════════════════════\n\n");
                                
                                // 从 POST body 中提取 token
                                token = extract_param(body_buffer, "tempToken");
                                if (!token) {
                                    token = extract_param(body_buffer, "token");
                                }
                                if (!token) {
                                    token = extract_param(body_buffer, "hwid_account");
                                }
                                if (!token) {
                                    token = extract_param(body_buffer, "access_token");
                                }
                                
                                free(body_buffer);
                            }
                        } else {
                            // 数据已经全部接收
                            printf("\n[DEBUG] POST Body:\n");
                            printf("═══════════════════════════════════════\n");
                            printf("%s\n", body_start);
                            printf("═══════════════════════════════════════\n\n");
                            
                            token = extract_param(body_start, "tempToken");
                            if (!token) {
                                token = extract_param(body_start, "token");
                            }
                            if (!token) {
                                token = extract_param(body_start, "hwid_account");
                            }
                            if (!token) {
                                token = extract_param(body_start, "access_token");
                            }
                        }
                    }
                }
            }
        } else {
            // GET 请求 - 从 URL 参数提取
            token = extract_param(buffer, "tempToken");
            if (!token) {
                token = extract_param(buffer, "token");
            }
            if (!token) {
                token = extract_param(buffer, "access_token");
            }
            if (!token) {
                token = extract_param(buffer, "hwid_account");
            }
        }
        
        // 方法2: 从 Cookie 提取（如果上面没找到）
        if (!token) {
            const char *cookie_start = strstr(buffer, "Cookie:");
            if (cookie_start) {
                const char *hwid_start = strstr(cookie_start, "hwid_account=");
                if (hwid_start) {
                    hwid_start += strlen("hwid_account=");
                    const char *hwid_end = strstr(hwid_start, ";");
                    if (!hwid_end) {
                        hwid_end = strstr(hwid_start, "\r\n");
                    }
                    if (hwid_end) {
                        size_t len = hwid_end - hwid_start;
                        token = malloc(len + 1);
                        if (token) {
                            strncpy(token, hwid_start, len);
                            token[len] = '\0';
                        }
                    }
                }
            }
        }
        
        if (token && strlen(token) > 10) {
            printf("[DEBUG] ✓ Token found: %.50s...\n\n", token);
            
            g_received_token = token;
            g_server_should_stop = 1;
            
            // 发送成功页面
            send(client_sock, SUCCESS_PAGE, strlen(SUCCESS_PAGE), 0);
        } else {
            printf("[DEBUG] ✗ Token not found in request\n\n");
            
            // 发送等待页面
            const char *waiting_page = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<!DOCTYPE html>"
                "<html>"
                "<head><meta charset='utf-8'><title>Horpkg Auth</title></head>"
                "<body>"
                "<h1>Waiting for authentication...</h1>"
                "<p>Please complete login in the browser.</p>"
                "<script>setTimeout(function(){ location.reload(); }, 2000);</script>"
                "</body>"
                "</html>";
            send(client_sock, waiting_page, strlen(waiting_page), 0);
            
            if (token) free(token);
        }
    }
    
    close(client_sock);
}

// 服务器线程
static void* server_thread(void *arg) {
    int server_sock = *(int*)arg;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    time_t start_time = time(NULL);
    
    while (!g_server_should_stop) {
        // 检查超时
        if (time(NULL) - start_time > TIMEOUT_SECONDS) {
            print_error("Authentication timeout (5 minutes)");
            break;
        }
        
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_sock >= 0) {
            handle_request(client_sock);
        }
    }
    
    return NULL;
}

// 启动服务器并获取 token
char* http_server_get_oauth_token(int port, const char *auth_url) {
    int server_sock;
    struct sockaddr_in server_addr;
    
    // 创建 socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        print_error("Failed to create socket");
        return NULL;
    }
    
    // 设置 SO_REUSEADDR
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        print_error("Failed to bind port");
        close(server_sock);
        return NULL;
    }
    
    // 监听
    if (listen(server_sock, 5) < 0) {
        print_error("Failed to listen");
        close(server_sock);
        return NULL;
    }
    
    // ← 修复：先格式化字符串
    char success_msg[128];
    snprintf(success_msg, sizeof(success_msg), "Local OAuth server started on port %d", port);
    print_success(success_msg);
    
    // 决定是自动打开浏览器还是打印URL
    if (g_config.manual_auth) {
        printf("\n%s[MANUAL AUTHENTICATION]%s\n", COLOR_CYAN, COLOR_RESET);
        printf("Please open the following URL in your browser:\n\n");
        printf("%s%s%s\n\n", COLOR_BOLD, auth_url, COLOR_RESET);
        printf("NOTE: You must ensure that port %d is accessible from the browser machine.\n", port);
        printf("      (e.g., if using SSH, forward the port: ssh -L %d:localhost:%d ...)\n\n", port, port);
    } else {
        // 打开浏览器
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "hdc-lite shell aa start "
                 "-A ohos.want.action.viewData "
                 "-e entity.system.browsable "
                 "-U \"%s\" "
                 "2>&1 >/dev/null",
                 auth_url);
        
        print_info("Opening browser on device...");
        system(cmd);
        print_success("Browser opened. Please login in the browser.");
    }

    printf("\n");
    
    // 启动服务器线程
    pthread_t tid;
    g_server_should_stop = 0;
    g_received_token = NULL;
    
    if (pthread_create(&tid, NULL, server_thread, &server_sock) != 0) {
        print_error("Failed to create server thread");
        close(server_sock);
        return NULL;
    }
    
    // 显示等待动画并支持手动输入
    printf("%sWaiting for authentication...%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  1. If using SSH, ensure port forwarding is active: %sssh -L %d:localhost:%d ...%s\n", COLOR_BOLD, port, port, COLOR_RESET);
    printf("  2. Or, copy the full Redirect URL from your browser (even if it fails to load) and %spaste it here%s:\n\n", COLOR_GREEN, COLOR_RESET);
    fflush(stdout);
    
    const char *spinner = "|/-\\";
    int spinner_idx = 0;
    
    // 设置 stdin 为非缓冲模式需要在 main 中处理，或者使用 select 轮询
    fd_set readfds;
    struct timeval tv;
    char input_buffer[4096];

    while (!g_server_should_stop) {
        // 打印 Spinner
        printf("\r%c Waiting... (Paste URL here if needed) ", spinner[spinner_idx]);
        fflush(stdout);
        spinner_idx = (spinner_idx + 1) % 4;

        // 使用 select 检查 stdin 是否有输入
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout (控制 spinner 速度)

        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            // 有用户输入
            if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
                // 移除换行符
                input_buffer[strcspn(input_buffer, "\n")] = 0;
                
                // 尝试提取 Token
                char *manual_token = extract_param(input_buffer, "tempToken");
                if (!manual_token) manual_token = extract_param(input_buffer, "token");
                if (!manual_token) manual_token = extract_param(input_buffer, "hwid_account");
                if (!manual_token) manual_token = extract_param(input_buffer, "access_token");

                // 如果用户直接粘贴的是纯 Token (假设长度足够长且没有 param= 前缀)
                // 这里做一个简单的启发式判断：如果没找到 key=value，但字符串很长，可能就是 token 本身
                if (!manual_token && strlen(input_buffer) > 32) {
                     // 简单判断，避免误操作
                     manual_token = strdup(input_buffer);
                }

                if (manual_token) {
                    printf("\n%s✓ Manual input detected!%s\n", COLOR_GREEN, COLOR_RESET);
                    g_received_token = manual_token;
                    g_server_should_stop = 1;
                    break;
                } else {
                    printf("\n%s✗ Invalid input. Please paste the full URL containing 'tempToken='.%s\n", COLOR_RED, COLOR_RESET);
                }
            }
        }
        // 如果 select 超时 (ret == 0)，循环继续，刷新 spinner
    }
    
    printf("\r \n"); // 清除 spinner 行
    
    // 等待线程结束
    pthread_join(tid, NULL);
    close(server_sock);
    
    if (g_received_token) {
        print_success("Token received automatically!");
        printf("\n");
        return g_received_token;
    } else {
        print_error("Failed to receive token");
        return NULL;
    }
}