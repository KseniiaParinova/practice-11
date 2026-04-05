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