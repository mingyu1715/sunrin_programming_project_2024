#include <openssl/evp.h>
#include <openssl/err.h> 
#include <stdlib.h>
#include "../include/aes.h"
#include "../include/utils.h"
// AES 256 비트 키의 길이
#define AES_KEY_LEN 32

// IV 길이 (16바이트)
#define IV_LEN 16

// AES 암호화 함수
int encrypt(unsigned char* plaintext, int plaintext_len, unsigned char* key, unsigned char* iv, unsigned char* ciphertext) {
    EVP_CIPHER_CTX* ctx;
    int len;
    int ciphertext_len;
    
    printf("Key: ");
    for (int i = 0; i < AES_KEY_LEN; i++) {
        printf("%02x ", key[i]);
    }
    printf("\n");

    printf("IV: ");
    for (int i = 0; i < IV_LEN; i++) {
        printf("%02x ", iv[i]);
    }
    printf("\n");

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) handleErrors();
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) handleErrors();
    ciphertext_len = len;
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}


// 복호화 함수 개선: 디버깅용 출력 추가
int decrypt(unsigned char* ciphertext, int ciphertext_len, unsigned char* key, unsigned char* iv, unsigned char* plaintext) {
    EVP_CIPHER_CTX* ctx;
    int len;
    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) {
        handleErrors();
    }

    printf("Key: ");
    for (int i = 0; i < AES_KEY_LEN; i++) {
        printf("%02x ", key[i]);
    }
    printf("\n");

    printf("IV: ");
    for (int i = 0; i < IV_LEN; i++) {
        printf("%02x ", iv[i]);
    }
    printf("\n");

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        handleErrors();
    }

    // 복호화 실행
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        handleErrors();
    }
    plaintext_len = len;

    // 패딩 처리
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        unsigned long err_code = ERR_get_error();
        char err_msg[120];
        ERR_error_string_n(err_code, err_msg, sizeof(err_msg));
        printf("Decryption error: %s\n", err_msg);
        EVP_CIPHER_CTX_free(ctx);
        return -1;  // 패딩 오류 발생 시 복호화 실패
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;  // 복호화된 평문 길이 반환
}
