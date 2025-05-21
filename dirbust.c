#include "dirbust.h"

FILE *wordlist_file;
mtx_t file_mutex;
const char *base_url = NULL;

size_t discard_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	return size * nmemb;
}

size_t save_to_memory(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size *nmemb;
	MemoryStruct *mem = (MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) return 0;

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void extract_hostname(const char *url, char *hostname, size_t maxlen) {
	const char *start = strstr(url, "://");
	start = start ? start + 3 : url;

	const char *end = strchr(start, '/');
	size_t len = end ? (size_t)(end - start) : strlen(start);

	if (len >= maxlen) {
		len = maxlen - 1;
	}
	strncpy(hostname, start, len);
	hostname[len] = '\0';

	char *colon = strchr(hostname, ':');
	if (colon) {
		*colon = '\0';
	}
}

void create_directories(const char *path) {
	char tmp[2048];
	snprintf(tmp, sizeof(tmp), "%s", path);

	for (char *p = tmp + strlen("output") + 1; *p; p++) {
		if (*p == '/') {
			*p = 0;
			mkdir(tmp, 0755);
			*p = '/';
		}
	}
}

void save_html(const char *base_url, const char *url_path, const char *html) {
	char hostname[256];
	extract_hostname(base_url, hostname, sizeof(hostname));

	char filepath[2048];
	snprintf(filepath, sizeof(filepath), "output/%s/%s",hostname, url_path);
	if (filepath[strlen("output/") + strlen(hostname)] == '/') {
		memmove(&filepath[strlen("output/") + strlen(hostname)],
			&filepath[strlen("output/") + strlen(hostname) + 1],
			strlen(&filepath[strlen("output/") + strlen(hostname) + 1]) + 1);
	}
	if (filepath[strlen(filepath) - 1] == '/') {
		strcat(filepath, "index.html");
	} else {
		strcat(filepath, ".html");
	}

	create_directories(filepath);

	FILE *f = fopen(filepath, "w");
	if (f) {
		fwrite(html, 1, strlen(html), f);
		fclose(f);
		printf("    [>] Saved to: %s\n", filepath);
	} else {
		perror("    [!] Failed to save HTML!");
	}
}
void check_url(const char *base_url, const char *path) {
	CURL *curl;
	CURLcode res;
	char full_url[2048];

	snprintf(full_url, sizeof(full_url), "%s/%s", base_url, path);

	MemoryStruct chunk = { .memory = malloc(1), .size = 0 };

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, full_url);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_callback);

		res = curl_easy_perform(curl);

		long response_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		if (res == CURLE_OPERATION_TIMEDOUT) {
			fprintf(stderr, "[-] Timeout: %s\n", full_url);
		}
		if (res == CURLE_OK && response_code == 200) {
			printf("[+] Found: %s (%ld)\n", full_url, response_code);
			save_html(base_url, path, chunk.memory);
		}
		if (res == CURLE_OK && response_code == 403) {
			printf("[/] Found: %s (%ld)\n", full_url, response_code);
			save_html(base_url, path, chunk.memory);
		}
	
		free(chunk.memory);
		curl_easy_cleanup(curl);
	}
}

void *thread_func(void *arg) {
	char line[MAX_LINE];

	while (1) {
		mtx_lock(&file_mutex);
		if (!fgets(line, sizeof(line), wordlist_file)) {
			mtx_unlock(&file_mutex);
			break;
		}
		mtx_unlock(&file_mutex);

		line[strcspn(line, "\r\n")] = 0;
		if (strlen(line) > 0) {
			check_url(base_url, line);
		}
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <base_url> <wordlist>\n", argv[0]);
		return 1;
	}

	base_url = argv[1];
	const char *wordlist_path = argv[2];
	int thread_count = atoi(argv[3]);

	if (thread_count <1 || thread_count > MAX_THREADS) {
		fprintf(stderr, "Please choose a thread count between 1 and %d\n", MAX_THREADS);
		return 1;
	}

	wordlist_file = fopen(wordlist_path, "r");
	if (!wordlist_file) {
		perror("Failed to open wordlist");
		return 1;
	}

	mtx_init(&file_mutex, NULL);
	curl_global_init(CURL_GLOBAL_ALL);

	thrd_t threads[MAX_THREADS];

	for (int i = 0; i<thread_count; i++) {
		thrd_create(&threads[i], NULL, thread_func, NULL);
	}

	for (int i = 0; i<thread_count; i++) {
		thrd_join(threads[i], NULL);
	}

	curl_global_cleanup();
	mtx_destroy(&file_mutex);
	fclose(wordlist_file);

	return 0;
}
