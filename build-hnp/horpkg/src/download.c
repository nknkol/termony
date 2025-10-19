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

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfile, "wb");
        if (fp == NULL) {
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);


        res = curl_easy_perform(curl);
        printf("\n");

        fclose(fp);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return -1;
        }

        curl_easy_cleanup(curl);
    }
    return 0;
}