#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
//#include "../include/key_iv.h"
#include "../include/aes.h"
#include "../include/utils.h"
#include "../include/sha.h"

int main() {
    // 비밀번호 입력 받기
    unsigned char password[32];
    printf("비밀번호를 입력하세요: ");
    fgets((char*)password, sizeof(password), stdin);
    password[strcspn((char*)password, "\n")] = '\0';  // 개행 문자 제거

    // 솔트 생성 및 출력
    unsigned char salt[16];
    make_salt(salt, sizeof(salt));
    printf("생성된 솔트: ");
    print_hex(salt, sizeof(salt));

    // 사용자가 직접 솔트를 입력하도록 요청
    unsigned char user_salt[16];
    char salt_input[33];  // 16 바이트 * 2자리 16진수 + null terminator

    printf("\n솔트를 입력하세요 (16 바이트, 32자리 16진수, 예: bf2c3c3cf29586fd56089871b46128cb): ");
    fgets(salt_input, sizeof(salt_input), stdin);  // 사용자가 입력한 32자리 16진수를 한 줄로 읽기
    salt_input[strcspn(salt_input, "\n")] = '\0';  // 개행 문자 제거

    // 입력한 16진수 문자열을 바이트 배열로 변환
    for (int i = 0; i < 16; i++) {
        sscanf(&salt_input[i * 2], "%2hhx", &user_salt[i]);  // 2자리씩 읽어서 user_salt에 저장
    }
    /*
    [[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]


    salt 와 비번 같아도 hash이상함 수정요망
    

    [[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]

    */
    // 사용자로부터 받은 솔트와 변환된 바이트 배열 출력
    printf("\n입력받은 솔트 (16진수): %s\n", salt_input);
    printf("바이트 배열로 변환된 솔트: ");
    print_hex(user_salt, sizeof(user_salt));

    // 비밀번호와 사용자가 입력한 솔트로부터 키를 생성
    unsigned char key[32];
    calculate_hash(password, user_salt, key);
    
    // 생성된 키 출력
    printf("생성된 키 (SHA-256 해시): ");
    print_hex(key, sizeof(key));


    // IV 설정
    uint8_t iv[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    // 파일에서 평문을 읽어오기 전에 파일 크기 계산
    size_t plaintext_len = get_file_size("./input.txt");
    if (plaintext_len == 0) {
        printf("파일 크기가 0입니다. 평문을 읽을 수 없습니다.\n");
        return 1;
    }

    // 평문, 암호문, 복호문을 위한 동적 메모리 할당
    unsigned char* plaintext = malloc(plaintext_len);
    unsigned char* ciphertext = malloc(plaintext_len+16);  // 암호문도 평문 크기만큼 할당
    unsigned char* decryptedtext = malloc(plaintext_len+16);

    if (plaintext == NULL || ciphertext == NULL || decryptedtext == NULL) {
        printf("메모리 할당 실패\n");
        return 1;
    }

    // 파일에서 평문을 읽어옵니다
    size_t read_plaintext_len = read_file("./input.txt", plaintext, plaintext_len);

    // 평문을 암호화합니다
    int ciphertext_len = encrypt(plaintext, read_plaintext_len, key, iv, ciphertext);

    // 암호화된 데이터를 파일에 저장 (encrypted_files 폴더에 저장)
    char encrypted_filename[100];
    generate_filename_with_timestamp(encrypted_filename, sizeof(encrypted_filename), "encrypted", "encrypted_files");
    write_file(encrypted_filename, ciphertext, ciphertext_len);

    // 출력할 파일을 엽니다
    FILE *output_file = fopen("log.txt", "a"); // "a" 모드로 열어 로그를 추가합니다
    if (output_file == NULL) {
        printf("파일 열기에 실패했습니다.\n");
        free(plaintext);
        free(ciphertext);
        free(decryptedtext);
        return 1;
    }

    // 현재 시간 기록
    char timestamp[20];
    generate_timestamp(timestamp, sizeof(timestamp));

    // 암호화된 데이터와 관련된 정보를 파일에 기록
    fprintf(output_file, "[%s] 암호화된 데이터가 저장된 파일: %s\n", timestamp, encrypted_filename);
    write_hex_to_file(ciphertext, ciphertext_len, output_file);  // 암호화된 데이터의 헥사 값 기록

    // 암호화된 파일에서 다시 데이터를 읽어옵니다
    size_t read_ciphertext_len = read_file(encrypted_filename, ciphertext, ciphertext_len);

    // 데이터를 복호화합니다
    int decryptedtext_len = decrypt(ciphertext, read_ciphertext_len, key, iv, decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';  // 문자열 종료 문자를 추가

    // 복호화된 데이터를 새로운 파일에 저장 (decrypted_files 폴더에 저장)
    char decrypted_filename[100];
    generate_filename_with_timestamp(decrypted_filename, sizeof(decrypted_filename), "decrypted", "decrypted_files");
    write_file(decrypted_filename, decryptedtext, decryptedtext_len);

    // 복호화된 데이터 관련 정보를 파일에 기록 (복호화된 데이터 내용은 제외)
    fprintf(output_file, "[%s] 복호화된 데이터가 저장된 파일: %s\n", timestamp, decrypted_filename);

    // 파일을 닫습니다
    fclose(output_file);

    // 결과 메시지 출력
    printf("결과가 log.txt에 저장되었습니다.\n");

    // 동적 할당한 메모리 해제
    free(plaintext);
    free(ciphertext);
    free(decryptedtext);

    return 0;
}
