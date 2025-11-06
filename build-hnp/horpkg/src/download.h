#ifndef HORPKG_DOWNLOAD_H
#define HORPKG_DOWNLOAD_H

#include <stdio.h> // for FILE
#include <curl/curl.h> // for curl_off_t

// (从 download.c 移出的函数原型)
int download_file(const char *url, const char *outfile);

#endif // HORPKG_DOWNLOAD_H