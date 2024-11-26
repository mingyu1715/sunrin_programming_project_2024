#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <openssl/rand.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #define CREATE_FOLDER(folderName) _mkdir(folderName)
#else
    #include <sys/stat.h>
    #define CREATE_FOLDER(folderName) mkdir(folderName, 0755)
#endif

#include "../include/sha.h"

sqlite3* connect_db() {
    sqlite3 *db;
    if (sqlite3_open("users.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

void create_user_table(sqlite3 *db) {
    // users 테이블 생성 쿼리
    const char *user_sql = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT NOT NULL UNIQUE, "
        "password BLOB NOT NULL, "
        "salt BLOB NOT NULL);";

// memos 테이블 생성 쿼리
    const char *memo_sql = 
        "CREATE TABLE IF NOT EXISTS memos ("
        "username TEXT NOT NULL, " // 사용자 이름 (외래 키)
        "memo_id INTEGER NOT NULL, " // 메모 ID
        "title TEXT NOT NULL, " // 메모 제목
        "iv BLOB NOT NULL, "  // IV 컬럼
        "PRIMARY KEY (username, memo_id), " // username + memo_id 조합으로 기본 키 설정
        "FOREIGN KEY (username) REFERENCES users (username) ON DELETE CASCADE);"; // 외래 키 관계 정의


    char *err_msg = 0;

    // users 테이블 생성
    int rc = sqlite3_exec(db, user_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in users table creation: %s\n", err_msg);
        sqlite3_free(err_msg);
    }

    // memos 테이블 생성
    rc = sqlite3_exec(db, memo_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error in memos table creation: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}


// 디렉토리 생성 함수
void create_folder(const char *path) {
    struct stat st;

    // 경로 존재 여부 확인
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            //printf("디렉토리가 이미 존재합니다: %s\n", path);
        } else {
            //perror("동일한 이름의 파일이 존재합니다: %s\n", path);
        }
    } else {
        if (mkdir(path, 0755) == 0) {
            //printf("디렉토리 생성 성공: %s\n", path);
        } else {
            //perror("디렉토리 생성 실패");
        }
    }
}


void add_user(sqlite3 *db, const char *username, const char *password) {
    sqlite3_stmt *stmt;
    unsigned char salt[16];
    unsigned char hash[32];

    // salt 자동 생성
    make_salt(salt, 16);

    // 비밀번호 해시 계산
    calculate_hash((unsigned char*)password, salt, hash);

    // 사용자 존재 여부 확인
    const char *check_user_sql = "SELECT COUNT(*) FROM users WHERE username = ?";
    sqlite3_prepare_v2(db, check_user_sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int user_count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (user_count > 0) {
            printf("-3\n");  // 사용자 존재 시 -1 리턴
            return;
        }
    }


    // 사용자 정보를 데이터베이스에 추가
    const char *sql = "INSERT INTO users (username, password, salt) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, hash, 32, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, salt, 16, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // 사용자 디렉토리 생성
    char path[256];
    snprintf(path, sizeof(path), "data/userdata/%s", username);
    create_folder(path);
    printf("1\n");  // 사용자 추가 성공
}

// 로그인 확인
int check_login(sqlite3 *db, const char *username, const char *password) {
    sqlite3_stmt *stmt;
    unsigned char salt[16];
    unsigned char hash[32];

    // 사용자 존재 여부 확인
    const char *check_user_sql = "SELECT COUNT(*) FROM users WHERE username = ?";
    sqlite3_prepare_v2(db, check_user_sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int user_count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        if (user_count == 0) {
            printf("-2\n");  // 사용자 없음 메시지 출력
            return -2;  // 사용자 없음
        }
    } else {
        sqlite3_finalize(stmt);
        return -1;
    }

    // 사용자 정보 조회
    const char *sql = "SELECT salt, password FROM users WHERE username = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const void *stored_salt = sqlite3_column_blob(stmt, 0);
        memcpy(salt, stored_salt, 16);
        const void *stored_hash = sqlite3_column_blob(stmt, 1);
        calculate_hash((unsigned char*)password, salt, hash);
        if (memcmp(stored_hash, hash, 32) == 0) {
            sqlite3_finalize(stmt);
            printf("1\n");  // 로그인 성공 메시지 출력
            return 1;  // 성공
        }
    }
    sqlite3_finalize(stmt);
    printf("-1\n");  // 로그인 실패 메시지 출력
    return -1;  // 실패
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <username> <password> <action>\n", argv[0]);
        return -1;
    }

    sqlite3 *db = connect_db();
    if (db == NULL) {
        //printf("Failed to open database\n");
        return 1;
    }

    // 사용자 테이블 생성
    create_user_table(db);

    const char *username = argv[1];
    const char *password = argv[2];
    const char *action = argv[3];

    int result = -1;
    if (strcmp(action, "login") == 0) {
        result = check_login(db, username, password);
    } else if (strcmp(action, "register") == 0) {
        add_user(db, username, password);
        result = 1;
    }

    sqlite3_close(db);
    return result;
}
