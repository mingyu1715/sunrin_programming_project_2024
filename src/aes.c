source venv/bin/activate
#include <openssl/evp.h>
#include <openssl/err.h> 
#include <stdlib.h>
#include "../include/aes.h"
#include "../include/utils.h"

// AES 암호화 함수
int encrypt(unsigned char* plaintext, int plaintext_len, unsigned char* key, unsigned char* iv, unsigned char* ciphertext) {
    EVP_CIPHER_CTX* ctx;
    int len;
    int ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) handleErrors();
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) handleErrors();
    ciphertext_len = len;
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
    ciphertext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

// AES 복호화 함수
int decrypt(unsigned char* ciphertext, int ciphertext_len, unsigned char* key, unsigned char* iv, unsigned char* plaintext) {
    EVP_CIPHER_CTX* ctx;
    int len;
    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) handleErrors();
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) handleErrors();
    plaintext_len = len;
    // 패딩 관련 오류를 확인하고 적절히 처리
    // 복호화 과정에서 패딩 문제를 처리하기 위한 코드
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    unsigned long err_code = ERR_get_error();
    char err_msg[120];
    ERR_error_string_n(err_code, err_msg, sizeof(err_msg));
    printf("Decryption error: %s\n", err_msg);
    return -1;  // 패딩 오류 발생 시 복호화 실패
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}
