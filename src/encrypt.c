#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <openssl/rand.h>
#include <ctype.h>
#include "../include/sha.h"
#include "../include/aes.h"
#include "../include/utils.h"

#define AES_KEY_LEN 32   // AES-256 키 길이
#define IV_LEN 16        // 초기화 벡터 길이

// 사용자 데이터 가져오기
int fetch_user_data(sqlite3 *db, const char *username, unsigned char *password_hash, unsigned char *salt) {
    const char *sql = "SELECT password, salt FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    // SQL 준비
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 준비 실패: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    // 파라미터 바인딩
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    // SQL 실행
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        memcpy(password_hash, sqlite3_column_blob(stmt, 0), AES_KEY_LEN);  // 비밀번호 해시값
        memcpy(salt, sqlite3_column_blob(stmt, 1), IV_LEN);                // salt
        sqlite3_finalize(stmt);
        return 1; // 성공
    }

    fprintf(stderr, "사용자 데이터를 찾을 수 없습니다.\n");
    sqlite3_finalize(stmt);
    return 0; // 실패
}

// memos 테이블에 새 메모 추가
int add_memo_to_db(sqlite3 *db, const char *username, int memo_id, const char *title, unsigned char *iv) {
    const char *sql = "INSERT INTO memos (username, memo_id, title, iv) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 준비 실패: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, memo_id);
    sqlite3_bind_text(stmt, 3, title, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 4, iv, IV_LEN, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "SQL 실행 실패: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1; // 성공
}

// iv를 가져오는 함수 (암호문 복호화에 사용할 IV를 DB에서 가져옴)
int fetch_memo_iv(sqlite3 *db, const char *username, int memo_id, unsigned char *iv) {
    const char *sql = "SELECT iv FROM memos WHERE username = ? AND memo_id = ?;";
    sqlite3_stmt *stmt;

    // SQL 준비
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 준비 실패: %s\n", sqlite3_errmsg(db));
        return 0;  // 실패
    }

    // 파라미터 바인딩
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, memo_id);

    // SQL 실행
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // IV 값을 가져옴
        memcpy(iv, sqlite3_column_blob(stmt, 0), IV_LEN);
        sqlite3_finalize(stmt);
        return 1;  // 성공
    }

    fprintf(stderr, "메모 IV를 가져오지 못했습니다.\n");
    sqlite3_finalize(stmt);
    return 0;  // 실패
}

// 새 메모 ID 생성
int generate_memo_id(sqlite3 *db, const char *username) {
    const char *sql = "SELECT IFNULL(MAX(memo_id), 0) + 1 FROM memos WHERE username = ?;";
    sqlite3_stmt *stmt;
    int new_memo_id = 1;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 준비 실패: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        new_memo_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return new_memo_id;
}
int main(int argc, char *argv[]) {
    if (argc != 4 && argc != 5) {
        fprintf(stderr, "사용법: %s <username> <plaintext_file> <title|memo_id> [memo_id(수정할 경우)]\n", argv[0]);
        return 1;
    }

    const char *username = argv[1];
    const char *plaintext_file = argv[2];
    const char *title_or_id = argv[3];  // title 또는 memo_id
    const char *db_file = "users.db";

    sqlite3 *db;
    if (sqlite3_open(db_file, &db) != SQLITE_OK) {
        fprintf(stderr, "DB 열기 실패: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // 사용자 정보 가져오기
    unsigned char password_hash[AES_KEY_LEN];
    unsigned char salt[IV_LEN];
    if (!fetch_user_data(db, username, password_hash, salt)) {
        sqlite3_close(db);
        return 1;
    }

    int memo_id = -1;

    // title_or_id가 숫자인 경우, 수정하는 것이므로 memo_id로 처리
    if (isdigit(title_or_id[0])) {
        memo_id = atoi(title_or_id);  // 수정할 메모 ID
        if (memo_id <= 0) {
            fprintf(stderr, "잘못된 memo_id입니다.\n");
            sqlite3_close(db);
            return 1;
        }
    }

    // 새로운 메모를 추가하는 경우
    if (memo_id == -1) {
        // 새 메모 ID 생성
        memo_id = generate_memo_id(db, username);
        if (memo_id == -1) {
            fprintf(stderr, "메모 ID 생성 실패\n");
            sqlite3_close(db);
            return 1;
        }

        // 암호화 파일 이름을 memo_id.txt로 설정
        char ciphertext_file[256];
        snprintf(ciphertext_file, sizeof(ciphertext_file), "data/userdata/%s/%d.txt", username, memo_id);

        // 입력 파일 읽기
        FILE *fp = fopen(plaintext_file, "rb");
        if (!fp) {
            perror("입력 파일 열기 실패");
            sqlite3_close(db);
            return 1;
        }
        fseek(fp, 0, SEEK_END);
        size_t plaintext_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        unsigned char *plaintext = malloc(plaintext_len);
        if (!plaintext) {
            perror("메모리 할당 실패");
            fclose(fp);
            sqlite3_close(db);
            return 1;
        }
        fread(plaintext, 1, plaintext_len, fp);
        fclose(fp);

        // IV 생성
        unsigned char iv[IV_LEN];
        if (RAND_bytes(iv, IV_LEN) != 1) {
            fprintf(stderr, "IV 생성 실패\n");
            free(plaintext);
            sqlite3_close(db);
            return 1;
        }

        // AES 키 생성
        unsigned char key[AES_KEY_LEN];
        memcpy(key, password_hash, AES_KEY_LEN);

        // AES 암호화
        unsigned char *ciphertext = malloc(plaintext_len + IV_LEN); // 패딩 포함
        if (!ciphertext) {
            perror("메모리 할당 실패");
            free(plaintext);
            sqlite3_close(db);
            return 1;
        }

        int ciphertext_len = encrypt(plaintext, plaintext_len, key, iv, ciphertext);
        if (ciphertext_len <= 0) {
            fprintf(stderr, "암호화 실패\n");
            free(plaintext);
            free(ciphertext);
            sqlite3_close(db);
            return 1;
        }

        FILE *out_fp = fopen(ciphertext_file, "wb");
        if (!out_fp) {
            perror("출력 파일 열기 실패");
            free(plaintext);
            free(ciphertext);
            sqlite3_close(db);
            return 1;
        }
        fwrite(ciphertext, 1, ciphertext_len, out_fp);
        fclose(out_fp);

        // 메모 정보 DB에 추가
        if (!add_memo_to_db(db, username, memo_id, title_or_id, iv)) {
            fprintf(stderr, "메모 DB 저장 실패\n");
            free(plaintext);
            free(ciphertext);
            sqlite3_close(db);
            return 1;
        }

        printf("새로운 메모가 생성되었습니다. 메모 ID: %d, 파일: %s\n", memo_id, ciphertext_file);

        free(plaintext);
        free(ciphertext);
    }
    else {
        // 수정하려는 메모의 내용만 암호화하여 저장
        // 기존 암호화된 파일 이름으로 지정
        char ciphertext_file[256];
        snprintf(ciphertext_file, sizeof(ciphertext_file), "data/userdata/%s/%d.txt", username, memo_id);

        // 입력 파일 읽기 (평문 파일)
        FILE *fp = fopen(plaintext_file, "rb");
        if (!fp) {
            perror("입력 파일 열기 실패");
            sqlite3_close(db);
            return 1;
        }
        fseek(fp, 0, SEEK_END);
        size_t plaintext_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        unsigned char *plaintext = malloc(plaintext_len);
        if (!plaintext) {
            perror("메모리 할당 실패");
            fclose(fp);
            sqlite3_close(db);
            return 1;
        }
        fread(plaintext, 1, plaintext_len, fp);
        fclose(fp);

        // IV는 기존 암호문에서 가져오기
        unsigned char iv[IV_LEN];
        if (!fetch_memo_iv(db, username, memo_id, iv)) {
            fprintf(stderr, "IV 정보 가져오기 실패\n");
            free(plaintext);
            sqlite3_close(db);
            return 1;
        }

        // AES 키 생성
        unsigned char key[AES_KEY_LEN];
        memcpy(key, password_hash, AES_KEY_LEN);

        // 새 암호문 생성
        unsigned char *ciphertext = malloc(plaintext_len + IV_LEN); // IV 길이를 고려
        if (!ciphertext) {
            perror("메모리 할당 실패");
            free(plaintext);
            sqlite3_close(db);
            return 1;
        }

        // 암호화
        int ciphertext_len = encrypt(plaintext, plaintext_len, key, iv, ciphertext);
        if (ciphertext_len <= 0) {
            fprintf(stderr, "암호화 실패\n");
            free(plaintext);
            free(ciphertext);
            sqlite3_close(db);
            return 1;
        }

        // 기존 암호문 파일 덮어쓰기
        fp = fopen(ciphertext_file, "wb");
        if (!fp) {
            perror("파일 열기 실패");
            free(plaintext);
            free(ciphertext);
            sqlite3_close(db);
            return 1;
        }
        fwrite(ciphertext, 1, ciphertext_len, fp);
        fclose(fp);

        printf("메모 내용이 새로 암호화되어 저장되었습니다. 메모 ID: %d, 파일: %s\n", memo_id, ciphertext_file);

        free(plaintext);
        free(ciphertext);
    }

    sqlite3_close(db);
    return 0;
}
