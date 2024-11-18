#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include "../include/aes.h"
#include "../include/utils.h"
#include "../include/sha.h"


void make_salt(unsigned char* salt, size_t salt_len) {
    srand((unsigned int)time(NULL));
    for (size_t i = 0; i < salt_len; i++) {
        salt[i] = (unsigned char)(rand() % 256);
    }
}

void calculate_hash(const unsigned char* password, const unsigned char* salt, unsigned char* hash) {
    // OpenSSL SHA256_CTX 초기화
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);  // SHA256_CTX 초기화

    // salt와 password를 하나로 결합하여 해시 계산
    SHA256_Update(&sha256_ctx, salt, 16);  // salt를 먼저 추가
    SHA256_Update(&sha256_ctx, password, strlen((const char*)password));  // password 추가

    // 최종 해시 계산
    SHA256_Final(hash, &sha256_ctx);
}
