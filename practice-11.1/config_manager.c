#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cjson/cJSON.h>

char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(len + 1);
    if (data) {
        fread(data, 1, len, f);
        data[len] = '\0';
    }
    fclose(f);
    return data;
}

int write_file(const char *filename, const char *data) {
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    fputs(data, f);
    fclose(f);
    return 1;
}

cJSON *load_config(const char *filename) {
    char *json_str = read_file(filename);
    if (!json_str) return NULL;
    cJSON *config = cJSON_Parse(json_str);
    free(json_str);
    return config;
}

int save_config(const char *filename, cJSON *config) {
    char *json_str = cJSON_Print(config);
    if (!json_str) return 0;
    int result = write_file(filename, json_str);
    free(json_str);
    return result;
}

void print_config(cJSON *config) {
    cJSON *app_name = cJSON_GetObjectItem(config, "app_name");
    cJSON *version = cJSON_GetObjectItem(config, "version");
    cJSON *settings = cJSON_GetObjectItem(config, "settings");
    cJSON *allowed_hosts = cJSON_GetObjectItem(config, "allowed_hosts");
    
    printf("=== Конфигурация приложения ===\n\n");
    printf("Название: %s\n", app_name ? app_name->valuestring : "N/A");
    printf("Версия: %s\n", version ? version->valuestring : "N/A");
    
    printf("\n--- Настройки ---\n");
    if (settings) {
        cJSON *debug = cJSON_GetObjectItem(settings, "debug");
        cJSON *max_conn = cJSON_GetObjectItem(settings, "max_connections");
        cJSON *timeout = cJSON_GetObjectItem(settings, "timeout_seconds");
        cJSON *log_file = cJSON_GetObjectItem(settings, "log_file");
        
        printf("debug: %s\n", debug ? (debug->valueint ? "true" : "false") : "N/A");
        printf("max_connections: %d\n", max_conn ? max_conn->valueint : 0);
        printf("timeout_seconds: %d\n", timeout ? timeout->valueint : 0);
        printf("log_file: %s\n", log_file ? log_file->valuestring : "N/A");
    }
    
    printf("\n--- Разрешённые хосты ---\n");
    if (allowed_hosts && cJSON_IsArray(allowed_hosts)) {
        int size = cJSON_GetArraySize(allowed_hosts);
        for (int i = 0; i < size; i++) {
            cJSON *host = cJSON_GetArrayItem(allowed_hosts, i);
            printf("  - %s\n", host->valuestring);
        }
    }
}

int set_config_value(cJSON *config, const char *key, const char *value) {
    if (!config || !key || !value) return 0;
    cJSON *settings = cJSON_GetObjectItem(config, "settings");
    if (!settings) {
        settings = cJSON_CreateObject();
        cJSON_AddItemToObject(config, "settings", settings);
    }
    
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        int bool_val = (strcmp(value, "true") == 0);
        cJSON_DeleteItemFromObject(settings, key);
        cJSON_AddBoolToObject(settings, key, bool_val);
    } else if (isdigit(value[0]) || (value[0] == '-' && isdigit(value[1]))) {
        int int_val = atoi(value);
        cJSON_DeleteItemFromObject(settings, key);
        cJSON_AddNumberToObject(settings, key, int_val);
    } else {
        cJSON_DeleteItemFromObject(settings, key);
        cJSON_AddStringToObject(settings, key, value);
    }
    return 1;
}

void print_usage(const char *prog_name) {
    printf("Использование:\n");
    printf("  %s              - показать конфигурацию\n", prog_name);
    printf("  %s set <key> <value> - изменить настройку\n", prog_name);
    printf("\nПримеры:\n");
    printf("  %s set debug true\n", prog_name);
    printf("  %s set max_connections 200\n", prog_name);
}

int main(int argc, char *argv[]) {
    const char *config_file = "config.json";
    cJSON *config = load_config(config_file);
    if (!config) return 1;
    
    if (argc == 1) {
        print_config(config);
    } else if (argc == 4 && strcmp(argv[1], "set") == 0) {
        if (set_config_value(config, argv[2], argv[3])) {
            if (save_config(config_file, config)) {
                printf("Настройка '%s' изменена на '%s'\n", argv[2], argv[3]);
            }
        }
    } else {
        print_usage(argv[0]);
    }
    
    cJSON_Delete(config);
    return 0;
}