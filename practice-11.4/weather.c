#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sqlite3.h>
#include <cjson/cJSON.h>
#include "http.h"
#include "db.h"

#define DB_FILE "weather.db"
#define WEATHER_API_URL "https://wttr.in/%s?format=j1"

// Функция для кодирования пробелов в URL
char *url_encode(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *encoded = malloc(len * 3 + 1);
    if (!encoded) return NULL;
    
    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == ' ') {
            encoded[pos++] = '%';
            encoded[pos++] = '2';
            encoded[pos++] = '0';
        } else {
            encoded[pos++] = str[i];
        }
    }
    encoded[pos] = '\0';
    return encoded;
}

// Парсинг JSON от wttr.in
int parse_weather_json(const char *json_str, double *temp, double *feels_like, int *humidity,
                       double *wind_speed, char *wind_dir, char *condition) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "❌ Ошибка парсинга JSON: %s\n", cJSON_GetErrorPtr());
        return 0;
    }

    cJSON *current_array = cJSON_GetObjectItem(root, "current_condition");
    if (!current_array || !cJSON_IsArray(current_array)) {
        fprintf(stderr, "❌ Ошибка: в JSON нет поля 'current_condition'\n");
        cJSON_Delete(root);
        return 0;
    }

    cJSON *current = cJSON_GetArrayItem(current_array, 0);
    if (!current) {
        fprintf(stderr, "❌ Ошибка: массив 'current_condition' пуст\n");
        cJSON_Delete(root);
        return 0;
    }

    cJSON *temp_c = cJSON_GetObjectItem(current, "temp_C");
    if (temp_c) *temp = atof(temp_c->valuestring);

    cJSON *feels_like_c = cJSON_GetObjectItem(current, "FeelsLikeC");
    if (feels_like_c) *feels_like = atof(feels_like_c->valuestring);

    cJSON *humidity_json = cJSON_GetObjectItem(current, "humidity");
    if (humidity_json) *humidity = atoi(humidity_json->valuestring);

    cJSON *wind_speed_json = cJSON_GetObjectItem(current, "windspeedKmph");
    if (wind_speed_json) *wind_speed = atof(wind_speed_json->valuestring);

    cJSON *wind_dir_json = cJSON_GetObjectItem(current, "winddir16Point");
    if (wind_dir_json) strcpy(wind_dir, wind_dir_json->valuestring);

    cJSON *weather_desc_array = cJSON_GetObjectItem(current, "weatherDesc");
    if (weather_desc_array && cJSON_IsArray(weather_desc_array)) {
        cJSON *desc_obj = cJSON_GetArrayItem(weather_desc_array, 0);
        if (desc_obj) {
            cJSON *desc_value = cJSON_GetObjectItem(desc_obj, "value");
            if (desc_value) strcpy(condition, desc_value->valuestring);
        }
    }

    cJSON_Delete(root);
    return 1;
}

void print_current_weather(const char *city, double temp, double feels_like, int humidity,
                           double wind_speed, const char *wind_dir, const char *condition) {
    printf("\n╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          ПОГОДА В ГОРОДЕ %-32s ║\n", city);
    printf("╠══════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ 🌡️  Температура:     %.1f°C (ощущается как %.1f°C)                      ║\n", temp, feels_like);
    printf("║ 💧 Влажность:       %d%%                                                  ║\n", humidity);
    printf("║ 💨 Ветер:           %.1f км/ч (%s)                                       ║\n", wind_speed, wind_dir);
    printf("║ ☁️  Условие:         %s                                                  ║\n", condition);
    printf("╚══════════════════════════════════════════════════════════════════════════╝\n");
}

void print_usage(const char *prog_name) {
    printf("\n╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         WEATHER APP v1.0                                 ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Использование:                                                           ║\n");
    printf("║   %s fetch <город>     - получить погоду и сохранить в БД                ║\n", prog_name);
    printf("║   %s history           - показать историю запросов                       ║\n", prog_name);
    printf("║   %s last              - показать последнюю запись                       ║\n", prog_name);
    printf("╠══════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Примеры:                                                                ║\n");
    printf("║   %s fetch Moscow                                                       ║\n", prog_name);
    printf("║   %s fetch \"St Petersburg\"                                              ║\n", prog_name);
    printf("║   %s history                                                            ║\n", prog_name);
    printf("║   %s last                                                               ║\n", prog_name);
    printf("╚══════════════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Инициализация базы данных
    sqlite3 *db;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) {
        fprintf(stderr, "❌ Ошибка открытия БД: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    if (!db_init(db)) {
        db_close(db);
        return 1;
    }

    // Инициализация libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    int result = 0;

    if (strcmp(argv[1], "fetch") == 0 && argc >= 3) {
        char *city_encoded = url_encode(argv[2]);
        if (!city_encoded) {
            fprintf(stderr, "❌ Ошибка кодирования названия города\n");
            curl_global_cleanup();
            db_close(db);
            return 1;
        }

        char url[512];
        snprintf(url, sizeof(url), WEATHER_API_URL, city_encoded);
        free(city_encoded);

        printf("\n📡 Запрос погоды для города: %s\n", argv[2]);

        HttpResponse resp = {NULL, 0};
        if (http_get(url, &resp)) {
            double temp = 0, feels_like = 0, wind_speed = 0;
            int humidity = 0;
            char wind_dir[20] = "";
            char condition[100] = "";

            if (parse_weather_json(resp.data, &temp, &feels_like, &humidity, 
                                   &wind_speed, wind_dir, condition)) {
                print_current_weather(argv[2], temp, feels_like, humidity, 
                                      wind_speed, wind_dir, condition);
                db_save_weather(db, argv[2], temp, feels_like, humidity, 
                                wind_speed, wind_dir, condition);
            } else {
                fprintf(stderr, "❌ Ошибка при обработке данных от сервера\n");
                result = 1;
            }
        } else {
            fprintf(stderr, "❌ Не удалось выполнить HTTP-запрос\n");
            result = 1;
        }
        http_response_free(&resp);
    } else if (strcmp(argv[1], "history") == 0) {
        db_get_history(db);
    } else if (strcmp(argv[1], "last") == 0) {
        db_get_last(db);
    } else {
        print_usage(argv[0]);
        result = 1;
    }

    curl_global_cleanup();
    db_close(db);
    return result;
}