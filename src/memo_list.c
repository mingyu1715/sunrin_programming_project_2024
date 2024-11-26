#include <stdio.h>
#include <sqlite3.h>

int callback(void *data, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        return 1;
    }

    const char *username = argv[1];
    sqlite3 *db;
    sqlite3_stmt *stmt;
    //char *errMsg = 0;
    int rc;

    rc = sqlite3_open("users.db", &db);
    if (rc) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    const char *sql = "SELECT title FROM memos WHERE username = ?;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // 바인딩된 값으로 username 추가
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    // 쿼리 실행 및 결과 처리
    //printf("Memos for username '%s':\n", username);
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(stmt, 0));
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error during execution: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}
