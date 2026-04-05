# Практическая работа 11: Работа со сторонними библиотеками
**Студент:** Сергеева Карина Алексеевна
**Группа:** 1зб_ИВТ(ускор.)/25
**Дата:** 03 апреля 2026 г.
---
## Задание 11.1: Работа с JSON (cJSON)
### Постановка задачи
Напишисать программу для работы с конфигурационным файлом в формате JSON.
### Список идентификаторов
| Имя | Тип | Описание |
|-----|-----|----------|
| cJSON | struct | Структура для работы с JSON |
| load_config | cJSON* (*)(const char*) | Загрузка конфигурации из файла |
| save_config | int (*)(const char*, cJSON*) | Сохранение конфигурации в файл |
| set_config_value | int (*)(cJSON*, const char*, const char*) | Изменение значения настройки |
### Код программы
config.json
```c
{
    "app_name": "MyApp",
    "version": "1.0",
    "settings": {
        "debug": false,
        "max_connections": 100,
        "timeout_seconds": 30,
        "log_file": "/var/log/myapp.log"
    },
    "allowed_hosts": ["localhost", "127.0.0.1"]
}
```
config_manager.c
```c
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
```
Makefile
```c
CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs libcjson)

TARGET = config_manager

all: $(TARGET)

$(TARGET): config_manager.c
	$(CC) $(CFLAGS) config_manager.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
```
---
## Задание 11.2: HTTP-клиент (libcurl)
### Постановка задачи
Напишисать консольную утилиту для выполнения HTTP-запросов.
### Список идентификаторов
| Имя | Тип | Описание |
|-----|-----|----------|
| Response | struct | Структура для хранения HTTP-ответа |
| http_get | int (*)(const char*, Response*) | Выполнение GET запроса |
| http_post | int (*)(const char*, const char*, Response*) | Выполнение POST запроса |
| write_callback | size_t (*)(void*, size_t, size_t, void*) | Callback для записи данных |
### Код программы
http_client.c
```c
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
```
Makefile
```c
CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs libcurl)

TARGET = http_client

all: $(TARGET)

$(TARGET): http_client.c
	$(CC) $(CFLAGS) http_client.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) get https://httpbin.org/get
```
### Результаты работы
![Результат выполнения](practice-11/practice-11.2/screenshots/task11-2.png)
---
## Задание 11.3: База данных (SQLite)
### Постановка задачи
Напишисать программу для управления списком задач (TODO) с хранением в SQLite.
### Список идентификаторов
| Имя | Тип | Описание |
|-----|-----|----------|
| sqlite3 | struct | Дескриптор базы данных |
| sqlite3_stmt | struct | Подготовленный запрос |
| init_db | int (*)(sqlite3*) | Инициализация БД |
| add_task | int (*)(sqlite3*, const char*) | Добавление задачи |
| list_tasks | int (*)(sqlite3*, int) | Вывод списка задач |
### Код программы
todo.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define DB_FILE "todo.db"

int init_db(sqlite3 *db) {
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS tasks ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " title TEXT NOT NULL,"
        " done INTEGER DEFAULT 0,"
        " created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int add_task(sqlite3 *db, const char *title) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO tasks (title) VALUES (?)";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Ошибка добавления: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    printf("Задача добавлена (ID: %lld)\n", sqlite3_last_insert_rowid(db));
    return 1;
}

int list_tasks(sqlite3 *db, int show_done) {
    sqlite3_stmt *stmt;
    const char *sql = show_done
        ? "SELECT id, title, done, created_at FROM tasks ORDER BY done, id;"
        : "SELECT id, title, done, created_at FROM tasks WHERE done=0 ORDER BY id;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Ошибка: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    printf("\n%-4s %-3s %-30s %s\n", "ID", " ", "Задача", "Создано");
    printf("--------------------------------------------------------------\n");
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *title = (const char *)sqlite3_column_text(stmt, 1);
        int done = sqlite3_column_int(stmt, 2);
        const char *created = (const char *)sqlite3_column_text(stmt, 3);
        printf("%-4d [%c] %-30s %s\n", id, done ? 'x' : ' ', title, created ? created : "");
    }
    
    sqlite3_finalize(stmt);
    return 1;
}

int complete_task(sqlite3 *db, int id) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE tasks SET done = 1 WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        printf("Задача %d отмечена выполненной\n", id);
        return 1;
    }
    fprintf(stderr, "Задача %d не найдена\n", id);
    return 0;
}

int delete_task(sqlite3 *db, int id) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM tasks WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        printf("Задача %d удалена\n", id);
        return 1;
    }
    fprintf(stderr, "Задача %d не найдена\n", id);
    return 0;
}

void print_usage(const char *prog_name) {
    printf("Менеджер задач TODO\n\n");
    printf("Использование:\n");
    printf("  %s list              - показать невыполненные задачи\n", prog_name);
    printf("  %s list all          - показать все задачи\n", prog_name);
    printf("  %s add <title>       - добавить задачу\n", prog_name);
    printf("  %s done <id>         - отметить выполненной\n", prog_name);
    printf("  %s delete <id>       - удалить задачу\n", prog_name);
}

int main(int argc, char *argv[]) {
    sqlite3 *db;
    if (sqlite3_open(DB_FILE, &db) != SQLITE_OK) {
        fprintf(stderr, "Ошибка открытия БД: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    
    if (!init_db(db)) {
        sqlite3_close(db);
        return 1;
    }
    
    if (argc < 2) {
        print_usage(argv[0]);
        sqlite3_close(db);
        return 1;
    }
    
    if (strcmp(argv[1], "list") == 0) {
        int show_all = (argc >= 3 && strcmp(argv[2], "all") == 0);
        list_tasks(db, show_all);
    } else if (strcmp(argv[1], "add") == 0 && argc >= 3) {
        add_task(db, argv[2]);
    } else if (strcmp(argv[1], "done") == 0 && argc >= 3) {
        complete_task(db, atoi(argv[2]));
    } else if (strcmp(argv[1], "delete") == 0 && argc >= 3) {
        delete_task(db, atoi(argv[2]));
    } else {
        print_usage(argv[0]);
    }
    
    sqlite3_close(db);
    return 0;
}
```
Makefile
```c
CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs sqlite3)

TARGET = todo

all: $(TARGET)

$(TARGET): todo.c
	$(CC) $(CFLAGS) todo.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET) todo.db

run: $(TARGET)
	./$(TARGET) list
```
### Результаты работы
![Результат выполнения](practice-11/practice-11.3/screenshots/task11-3.png)
---
## Задание 11.4: Комбинированный проект
### Постановка задачи
Создайте программу weather, которая делает HTTP-запрос к API погоды, парсит JSON-ответ, сохраняет данные в SQLite, выводит историю запросов. .
### Список идентификаторов
| Имя | Тип | Описание |
|-----|-----|----------|
| HttpResponse | struct | Структура для хранения HTTP-ответа |
| http_get | int (*)(const char*, HttpResponse*) | Выполнение GET запроса |
| parse_weather_json | int (*)(const char*, double*, double*, int*, double*, char*, char*) | Парсинг JSON |
| db_save_weather | int (*)(sqlite3*, const char*, double, double, int, double, const char*, const char*) | Сохранение в БД |
| db_get_history | int (*)(sqlite3*) | Получение истории |
| db_get_last | int (*)(sqlite3*) | Получение последней записи |
### Код программы
weather.c
```c
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
```
http.h
```c
#ifndef HTTP_H
#define HTTP_H

typedef struct {
    char *data;
    size_t size;
} HttpResponse;

int http_get(const char *url, HttpResponse *resp);
void http_response_free(HttpResponse *resp);

#endif
```
http.c
```c
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
```
db.h
```c
#ifndef DB_H
#define DB_H

#include <sqlite3.h>

int db_init(sqlite3 *db);
int db_save_weather(sqlite3 *db, const char *city, double temp, double feels_like, 
                    int humidity, double wind_speed, const char *wind_dir, const char *condition);
int db_get_history(sqlite3 *db);
int db_get_last(sqlite3 *db);
void db_close(sqlite3 *db);

#endif
```
db.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "db.h"

int db_init(sqlite3 *db) {
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS weather ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " city TEXT NOT NULL,"
        " temperature REAL,"
        " feels_like REAL,"
        " humidity INTEGER,"
        " wind_speed REAL,"
        " wind_dir TEXT,"
        " condition TEXT,"
        " created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";
    
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "❌ SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int db_save_weather(sqlite3 *db, const char *city, double temp, double feels_like, 
                    int humidity, double wind_speed, const char *wind_dir, const char *condition) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO weather (city, temperature, feels_like, humidity, wind_speed, wind_dir, condition) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Prepare error: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, city, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, temp);
    sqlite3_bind_double(stmt, 3, feels_like);
    sqlite3_bind_int(stmt, 4, humidity);
    sqlite3_bind_double(stmt, 5, wind_speed);
    sqlite3_bind_text(stmt, 6, wind_dir, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, condition, -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "❌ Insert error: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    printf("✅ Данные о погоде сохранены (ID: %lld)\n", sqlite3_last_insert_rowid(db));
    return 1;
}

int db_get_history(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, city, temperature, feels_like, humidity, wind_speed, wind_dir, condition, created_at "
                      "FROM weather ORDER BY created_at DESC LIMIT 10;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Query error: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    printf("\n╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              ИСТОРИЯ ЗАПРОСОВ ПОГОДЫ (последние 10)                                         ║\n");
    printf("╠════╦═══════════════╦════════════╦════════════╦════════╦════════════╦════════════╦════════════════════════════╣\n");
    printf("║ ID ║ Город        ║ Температура║ Ощущается  ║ Влажн. ║ Ветер      ║ Направление║ Условие                    ║\n");
    printf("╠════╬═══════════════╬════════════╬════════════╬════════╬════════════╬════════════╬════════════════════════════╣\n");
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *city = (const char *)sqlite3_column_text(stmt, 1);
        double temp = sqlite3_column_double(stmt, 2);
        double feels = sqlite3_column_double(stmt, 3);
        int humidity = sqlite3_column_int(stmt, 4);
        double wind = sqlite3_column_double(stmt, 5);
        const char *wind_dir = (const char *)sqlite3_column_text(stmt, 6);
        const char *cond = (const char *)sqlite3_column_text(stmt, 7);
        
        printf("║ %-2d ║ %-13s ║ %7.1f°C ║ %7.1f°C ║ %5d%% ║ %7.1f ║ %-10s ║ %-26s ║\n",
               id, city, temp, feels, humidity, wind, wind_dir ? wind_dir : "N/A", cond ? cond : "N/A");
    }
    
    printf("╚════╩═══════════════╩════════════╩════════════╩════════╩════════════╩════════════╩════════════════════════════╝\n");
    
    sqlite3_finalize(stmt);
    return 1;
}

int db_get_last(sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT city, temperature, feels_like, humidity, wind_speed, wind_dir, condition, created_at "
                      "FROM weather ORDER BY created_at DESC LIMIT 1;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "❌ Query error: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    printf("\n╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          ПОСЛЕДНЯЯ ЗАПИСЬ                                ║\n");
    printf("╠════════════════╦═════════════════════════════════════════════════════════╣\n");
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *city = (const char *)sqlite3_column_text(stmt, 0);
        double temp = sqlite3_column_double(stmt, 1);
        double feels = sqlite3_column_double(stmt, 2);
        int humidity = sqlite3_column_int(stmt, 3);
        double wind = sqlite3_column_double(stmt, 4);
        const char *wind_dir = (const char *)sqlite3_column_text(stmt, 5);
        const char *cond = (const char *)sqlite3_column_text(stmt, 6);
        const char *date = (const char *)sqlite3_column_text(stmt, 7);
        
        printf("║ Город        ║ %-57s ║\n", city);
        printf("╠════════════════╬═════════════════════════════════════════════════════════╣\n");
        printf("║ Температура   ║ %.1f°C (ощущается как %.1f°C)                          ║\n", temp, feels);
        printf("║ Влажность     ║ %d%%                                                      ║\n", humidity);
        printf("║ Ветер         ║ %.1f км/ч (%s)                                         ║\n", wind, wind_dir);
        printf("║ Условие       ║ %-57s ║\n", cond);
        printf("║ Дата          ║ %-57s ║\n", date);
    } else {
        printf("║ ║ Нет данных                                                           ║\n");
    }
    
    printf("╚════════════════╩═════════════════════════════════════════════════════════╝\n");
    
    sqlite3_finalize(stmt);
    return 1;
}

void db_close(sqlite3 *db) {
    if (db) {
        sqlite3_close(db);
    }
}
```
Makefile
```c
CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs libcurl sqlite3 libcjson)

TARGET = weather

all: $(TARGET)

$(TARGET): weather.c http.c db.c
	$(CC) $(CFLAGS) weather.c http.c db.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET) weather.db

run: $(TARGET)
	./$(TARGET) fetch Moscow

history: $(TARGET)
	./$(TARGET) history

last: $(TARGET)
	./$(TARGET) last

.PHONY: all clean run history last
```
