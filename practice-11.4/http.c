#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "http.h"

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    HttpResponse *resp = (HttpResponse *)userdata;
    
    char *new_data = realloc(resp->data, resp->size + total + 1);
    if (!new_data) return 0;
    
    resp->data = new_data;
    memcpy(resp->data + resp->size, ptr, total);
    resp->size += total;
    resp->data[resp->size] = '\0';
    
    return total;
}

int http_get(const char *url, HttpResponse *resp) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;
    
    resp->data = malloc(1);
    if (!resp->data) {
        curl_easy_cleanup(curl);
        return 0;
    }
    resp->data[0] = '\0';
    resp->size = 0;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WeatherApp/1.0 (libcurl)");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "❌ HTTP error: %s\n", curl_easy_strerror(res));
        free(resp->data);
        resp->data = NULL;
        resp->size = 0;
        return 0;
    }
    return 1;
}

void http_response_free(HttpResponse *resp) {
    if (resp && resp->data) {
        free(resp->data);
        resp->data = NULL;
        resp->size = 0;
    }
}