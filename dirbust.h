#ifndef DIRBUSTER_H
#define DIRBUSTER_Ha
#endif

#include <stdio.h>
#include <stdlib.h>
#include "libs/universal/tinycthreads/tinycthread.h"
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#include "libs/windows/curl-8.13.0_5-win64-mingw/include/curl/curl.h"
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#include <curl/curl.h>
#endif

#define MAX_LINE 1024
#define MAX_THREADS 10

typedef struct MemoryStruct {
	char *memory;
	size_t size;
} MemoryStruct;

size_t discard_callback(void *contents, size_t size, size_t nmemb, void *userp);

size_t save_to_memory(void *contents, size_t size, size_t nmemb, void *userp);

void extract_hostname(const char *url, char *hostname, size_t maxlen);

void create_directories(const char *path);

void save_html(const char *base_url, const char *url_path, const char *html);

void check_url(const char *base_url, const char *path);

int thread_func(void *arg);
