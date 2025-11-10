#include "download.h" // [!] 包含新头文件
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "utils.h"

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    if (dltotal <= 0) {
        return 0;
    }

    int percent = (int)(dlnow * 100 / dltotal);
    printf("\r    Downloading [");
    int i = 0;
    for (; i <= percent / 10; i++) {
        printf("█");
    }
    for (; i < 10; i++) {
        printf(" ");
    }
    printf("] %d%%", percent);
    fflush(stdout);

    return 0;
}

int download_file(const char *url, const char *outfile) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char errbuf[CURL_ERROR_SIZE];
    curl = curl_easy_init();

    if (curl) {
        errbuf[0] = 0;
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

        fp = fopen(outfile, "wb");
        if (fp == NULL) {
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        const char* ca_path = "/data/service/hnp/horpkg-base.org/horpkg-base_1.0/etc/cacert.pem";
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        res = curl_easy_perform(curl);
        printf("\n"); // 确保进度条之后换行

        // 关闭文件
        fclose(fp);

        // 检查结果
        if (res != CURLE_OK) {
            if (strlen(errbuf)) {
                fprintf(stderr, "[ERROR] CURL error details: %s\n", errbuf);
            }
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_easy_cleanup(curl);

    } else {
        fprintf(stderr, "[ERROR] FAILED: curl_easy_init() returned NULL.\n");
        return -1;
    }
    return 0;
}
