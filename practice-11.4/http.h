#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char *data;
    size_t size;
} HttpResponse;

int http_get(const char *url, HttpResponse *resp);
void http_response_free(HttpResponse *resp);

#endif