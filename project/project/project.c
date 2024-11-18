#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "../kisa/KISA_SHA256.h"

void print_hex(unsigned char* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

void make_salt(unsigned char* salt, size_t salt_len) {
    srand((unsigned int)time(NULL));
    for (size_t i = 0; i < salt_len; i++) {
        salt[i] = (unsigned char)(rand() % 256);
    }
}

void calculate_hash(const unsigned char* password, const unsigned char* salt, unsigned char* hash) {
    size_t password_len = strlen((char*)password);
    unsigned char sp[48];

    memcpy(sp, salt, 16);
    memcpy(sp + 16, password, password_len);

    SHA256_Encrpyt(sp, 16 + password_len, hash);
}

int signup(sqlite3* db) {
    char username[50];
    char password[32];
    unsigned char salt[16];
    unsigned char hash[32];
    char query[512];
    char* err_msg = NULL;

    printf("Enter username: ");
    scanf("%49s", username);
    printf("Enter password: ");
    scanf("%31s", password);

    make_salt(salt, sizeof(salt));
    calculate_hash((unsigned char*)password, salt, hash);

    printf("Password hash: ");
    print_hex(hash, sizeof(hash));

    snprintf(query, sizeof(query),
        "INSERT INTO users (username, salt, password_hash) VALUES "
        "('%s', x'", username);

    for (size_t i = 0; i < sizeof(salt); i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", salt[i]);
        strcat(query, hex);
    }
    strcat(query, "', x'");

    for (size_t i = 0; i < sizeof(hash); i++) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", hash[i]);
        strcat(query, hex);
    }
    strcat(query, "');");

    int rc = sqlite3_exec(db, query, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        if (strstr(err_msg, "UNIQUE constraint failed") != NULL) {
            printf("�̹� �ִ� ���̵��Դϴ�.\n");
        }
        else {
            fprintf(stderr, "SQL error: %s\n", err_msg);
        }
        sqlite3_free(err_msg);
        return 0;
    }

    printf("����� '%s' ���������� ȸ������ �Ǿ����ϴ�.\n", username);
    return 1;
}

int login(sqlite3* db) {
    char username[50];
    char password[32];
    unsigned char salt[16];
    unsigned char hash[32];
    unsigned char computed_hash[32];
    sqlite3_stmt* stmt;

    printf("Enter username: ");
    scanf("%49s", username);
    printf("Enter password: ");
    scanf("%31s", password);

    const char* query = "SELECT salt, password_hash FROM users WHERE username = ?;";
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* db_salt = sqlite3_column_blob(stmt, 0);
        int db_salt_len = sqlite3_column_bytes(stmt, 0);
        const unsigned char* db_hash = sqlite3_column_blob(stmt, 1);
        int db_hash_len = sqlite3_column_bytes(stmt, 1);

        if (db_salt_len == 16 && db_hash_len == 32) {
            memcpy(salt, db_salt, 16);
            memcpy(hash, db_hash, 32);

            calculate_hash((unsigned char*)password, salt, computed_hash);

            printf("Computed password hash: ");
            print_hex(computed_hash, sizeof(computed_hash));

            if (memcmp(hash, computed_hash, 32) == 0) {
                printf("�α��� ����! ȯ���մϴ�! %s.\n", username);
                sqlite3_finalize(stmt);
                return 1;
            }
        }
    }

    printf("�α��� ����. �߸��� ���̵� �Ǵ� ��й�ȣ�Դϴ�. \n");
    sqlite3_finalize(stmt);
    return 0;
}

int main() {
    sqlite3* db;
    int rc;
    char* err_msg = NULL;

    rc = sqlite3_open("USERS.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    const char* sql_create_table =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "salt BLOB NOT NULL, "
        "password_hash BLOB NOT NULL);";

    rc = sqlite3_exec(db, sql_create_table, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    int choice;
    do {
        printf("\n1. Sign Up\n2. Log In\n3. Exit\nChoose an option: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1:
            signup(db);
            break;
        case 2:
            login(db);
            break;
        case 3:
            printf("�̿��� �ּż� �����մϴ�!\n");
            break;
        default:
            printf("Invalid choice. Try again.\n");
        }
    } while (choice != 3);

    sqlite3_close(db);
    return 0;
}
