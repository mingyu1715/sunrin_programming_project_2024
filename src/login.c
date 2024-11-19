#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <openssl/rand.h>
#include "../include/sha.h"

// SQLite3에서 DB 연결 함수
sqlite3* connect_db() {
    sqlite3 *db;
    int rc = sqlite3_open("users.db", &db);
    if (rc) {
        fprintf(stderr, "DB 열기 오류: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

// 사용자 테이블 생성
void create_user_table(sqlite3 *db) {
    char *err_msg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS users ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "username TEXT NOT NULL UNIQUE, "
                      "password BLOB NOT NULL, "
                      "salt BLOB NOT NULL);";
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "테이블 생성 오류: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

// 사용자 추가 (기존 사용자 대체)
void add_user(sqlite3 *db, const char *username, const char *password) {
    sqlite3_stmt *stmt;
    unsigned char salt[16];
    unsigned char hash[32];  // SHA-256 해시 크기를 32로 수정

    // salt 자동 생성
    make_salt(salt, 16);

    // 비밀번호 해시 계산
    calculate_hash((unsigned char*)password, salt, hash);

    // INSERT OR REPLACE를 사용하여 중복된 username이 있으면 기존 데이터를 덮어씁니다
    const char *sql = "INSERT OR REPLACE INTO users (username, password, salt) VALUES (?, ?, ?)";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("SQL 준비 오류: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, hash, 32, SQLITE_STATIC);  // SHA-256 해시 크기 32로 수정
    sqlite3_bind_blob(stmt, 3, salt, 16, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    /*
    if (rc != SQLITE_DONE) {
       printf("사용자 추가 오류: %s\n", sqlite3_errmsg(db));
    } else {
        printf("사용자 추가 완료\n");
    }
    */
    sqlite3_finalize(stmt);
}

// 로그인 확인
int check_login(sqlite3 *db, const char *username, const char *password) {
    sqlite3_stmt *stmt;
    unsigned char salt[16];
    unsigned char hash[32];  // SHA-256 해시 크기를 32로 수정

    // 사용자 정보 조회
    const char *sql = "SELECT salt, password FROM users WHERE username = ?";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        printf("SQL 준비 오류: %s\n", sqlite3_errmsg(db));
        return -1;  // 실패
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // salt 가져오기
        const void *stored_salt = sqlite3_column_blob(stmt, 0);
        memcpy(salt, stored_salt, 16);

        // 저장된 해시 가져오기
        const void *stored_hash = sqlite3_column_blob(stmt, 1);
        
        // 비밀번호와 salt를 결합하여 해시 계산
        calculate_hash((unsigned char*)password, salt, hash);

        // 해시 비교
        if (memcmp(stored_hash, hash, 32) == 0) {
            sqlite3_finalize(stmt);
            //printf("성공\n");
            printf("1");
            return 1;  // 로그인 성공
        }
        else {
            printf("-1\n");
        }
    }

    sqlite3_finalize(stmt);
    return -1;  // 로그인 실패
}

int main(int argc, char *argv[]) {
    sqlite3 *db = connect_db();
    if (db == NULL) {
        return 1;
    }

    // 사용자 테이블 생성
    create_user_table(db);

    // 'admin' 사용자 추가
    const char *username = "admin";
    const char *password = "admin";

    // 'admin' 사용자 추가
    add_user(db, username, password);

    username = "sunrin";
    password = "1234";

    // 'admin' 사용자 추가
    add_user(db, username, password);
    // 로그인 처리
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <username> <password>\n", argv[0]);
        return -1;
    }

    const char *input_username = argv[1];
    const char *input_password = argv[2];

    int result = check_login(db, input_username, input_password);
    //printf("%d\n", result);  // 로그인 결과 출력

    sqlite3_close(db);
    return result;  // 1이면 로그인 성공, -1이면 로그인 실패
}
