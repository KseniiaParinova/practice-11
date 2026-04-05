#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    char *data;
    size_t size;
} Response;

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    Response *resp = (Response *)userdata;
    
    char *new_data = realloc(resp->data, resp->size + total + 1);
    if (!new_data) return 0;
    
    resp->data = new_data;
    memcpy(resp->data + resp->size, ptr, total);
    resp->size += total;
    resp->data[resp->size] = '\0';
    
    return total;
}

int http_get(const char *url, Response *resp) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;
    
    resp->data = malloc(1);
    resp->data[0] = '\0';
    resp->size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Ошибка: %s\n", curl_easy_strerror(res));
        return 0;
    }
    return 1;
}

int http_post(const char *url, const char *data, Response *resp) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;
    
    resp->data = malloc(1);
    resp->data[0] = '\0';
    resp->size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Ошибка: %s\n", curl_easy_strerror(res));
        return 0;
    }
    return 1;
}

void print_usage(const char *prog_name) {
    printf("Использование:\n");
    printf("  %s get <url>                     - GET запрос\n", prog_name);
    printf("  %s post <url> <json>             - POST запрос\n", prog_name);
    printf("  %s get <url> -o <file>           - сохранить в файл\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    Response resp = {NULL, 0};
    int success = 0;
    
    if (strcmp(argv[1], "get") == 0) {
        success = http_get(argv[2], &resp);
    } else if (strcmp(argv[1], "post") == 0 && argc >= 4) {
        success = http_post(argv[2], argv[3], &resp);
    } else {
        print_usage(argv[0]);
        curl_global_cleanup();
        return 1;
    }
    
    if (success && resp.data) {
        const char *output_file = NULL;
        for (int i = 3; i < argc - 1; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                output_file = argv[i + 1];
                break;
            }
        }
        
        if (output_file) {
            FILE *f = fopen(output_file, "w");
            if (f) {
                fputs(resp.data, f);
                fclose(f);
                printf("Сохранено в %s (%zu байт)\n", output_file, resp.size);
            }
        } else {
            printf("%s\n", resp.data);
        }
    }
    
    free(resp.data);
    curl_global_cleanup();
    return success ? 0 : 1;
}