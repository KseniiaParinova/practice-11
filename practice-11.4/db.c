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