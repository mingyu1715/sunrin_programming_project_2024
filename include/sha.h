#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <stdio.h>

// Salt 생성 함수
void make_salt(unsigned char* salt, size_t salt_len);
// 비밀번호와 salt를 이용하여 SHA-256 해시 계산
void calculate_hash(const unsigned char* password, const unsigned char* salt, unsigned char* hash);

void derive_aes_key(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *key, size_t key_len);

#endif // HASH_UTILS_H
