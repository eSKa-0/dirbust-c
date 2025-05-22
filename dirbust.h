#include <stdio.h>
#include <stdlib.h>
#include "libs/universal/tinycthreads/tinycthread.h"
#include "libs/windows/curl-8.13.0_5-win64-mingw/include/curl/curl.h"
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1024
#define MAX_THREADS 10

typedef struct MemoryStruct {
	char *memory;
	size_t size;
} MemoryStruct;
