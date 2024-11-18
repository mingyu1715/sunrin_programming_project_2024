#ifndef AES_H
#define AES_H

#include <stdint.h>

// AES 암호화 및 복호화 함수 선언
int encrypt(unsigned char* plaintext, int plaintext_len, unsigned char* key, unsigned char* iv, unsigned char* ciphertext);
int decrypt(unsigned char* ciphertext, int ciphertext_len, unsigned char* key, unsigned char* iv, unsigned char* plaintext);

#endif
