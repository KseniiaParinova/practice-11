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