#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <openssl/rand.h>
#include "../include/sha.h"
#include "../include/aes.h"
#include "../include/utils.h"

#define AES_KEY_LEN 32   // AES-256 키 길이
#define IV_LEN 16        // 초기화 벡터 길이

// 사용자 데이터 가져오기 (username 기반)
int fetch_user_data(sqlite3 *db, const char *username, unsigned char *password_hash, unsigned char *salt) {
    const char *sql = "SELECT password, salt FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    // SQL 준비
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQL 준비 실패: %s\n", sqlite3_errmsg(db));
        return 0;  // 실패
    }

    // 파라미터 바인딩
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    // SQL 실행
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 비밀번호 해시와 salt를 가져옴
        memcpy(password_hash, sqlite3_column_blob(stmt, 0), AES_KEY_LEN);
        memcpy(salt, sqlite3_column_blob(stmt, 1), IV_LEN);
        sqlite3_finalize(stmt);
        return 1;  // 성공
    }

    fprintf(stderr, "사용자 데이터를 가져오지 못했습니다.\n");
    sqlite3_finalize(stmt);
    return 0;  // 실패
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

// 암호화된 파일 복호화
int decrypt_file(const char* ciphertext_file, const char* output_file, unsigned char* key, unsigned char* iv) {
    FILE *in_fp = fopen(ciphertext_file, "rb");
    if (!in_fp) {
        perror("파일 열기 실패");
        return -1;
    }

    // 암호문 읽기
    fseek(in_fp, 0, SEEK_END);
    long ciphertext_len = ftell(in_fp);  // 암호문 크기
    fseek(in_fp, 0, SEEK_SET);

    unsigned char *ciphertext = malloc(ciphertext_len);
    if (!ciphertext) {
        perror("메모리 할당 실패");
        fclose(in_fp);
        return -1;
    }
    fread(ciphertext, 1, ciphertext_len, in_fp);
    fclose(in_fp);

    printf("암호문 읽기 성공. 크기: %ld 바이트\n", ciphertext_len);

    // 복호화된 데이터 저장
    unsigned char *plaintext = malloc(ciphertext_len);  // 암호문 크기만큼 평문을 저장할 공간
    if (!plaintext) {
        perror("메모리 할당 실패");
        free(ciphertext);
        return -1;
    }

    // 복호화
    int decrypted_len = decrypt(ciphertext, ciphertext_len, key, iv, plaintext);
    if (decrypted_len == -1) {
        fprintf(stderr, "복호화 실패\n");
        free(ciphertext);
        free(plaintext);
        return -1;
    }

    printf("복호화 완료. 평문 크기: %d 바이트\n", decrypted_len);

    // 복호화된 평문을 파일에 저장
    FILE *out_fp = fopen(output_file, "wb");
    if (!out_fp) {
        perror("출력 파일 열기 실패");
        free(ciphertext);
        free(plaintext);
        return -1;
    }
    fwrite(plaintext, 1, decrypted_len, out_fp);
    fclose(out_fp);

    printf("복호화 완료. 파일: %s\n", output_file);

    free(ciphertext);
    free(plaintext);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "사용법: %s <username> <memo_id> <ciphertext_file> <output_file>\n", argv[0]);
        return 1;
    }

    const char *username = argv[1];
    int memo_id = atoi(argv[2]);
    const char *ciphertext_file = argv[3];
    const char *output_file = argv[4];
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

    printf("사용자 데이터 가져오기 성공\n");

    unsigned char iv[IV_LEN];
    if (!fetch_memo_iv(db, username, memo_id, iv)) {
        sqlite3_close(db);
        return 1;
    }

    printf("IV 값 가져오기 성공\n");

    // AES 키 생성
    unsigned char key[AES_KEY_LEN];
    memcpy(key, password_hash, AES_KEY_LEN);
    printf("AES 키 생성 완료\n");

    // 파일 복호화
    if (decrypt_file(ciphertext_file, output_file, key, iv) != 0) {
        fprintf(stderr, "파일 복호화 실패\n");
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}
