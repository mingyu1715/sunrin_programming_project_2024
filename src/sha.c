#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include "../include/KISA_SHA256.h"
//#include "../include/key_iv.h"
#include "../include/aes.h"
#include "../include/utils.h"
#include "../include/sha.h"


void make_salt(unsigned char* salt, size_t salt_len) {
    srand((unsigned int)time(NULL));
    for (size_t i = 0; i < salt_len; i++) {
        salt[i] = (unsigned char)(rand() % 256);
    }
}

    /*
    [[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]


    이 코드는 이상 없는듯?
    

    [[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]

    */

void calculate_hash(const unsigned char* password, const unsigned char* salt, unsigned char* hash) {
    size_t password_len = strlen((char*)password);
    unsigned char sp[48];

    memcpy(sp, salt, 16);
    memcpy(sp + 16, password, password_len);

    SHA256_Encrpyt(sp, 16 + password_len, hash);
}