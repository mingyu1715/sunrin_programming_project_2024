#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/err.h>
#include "../include/utils.h"

size_t get_file_size(const char *filename) {
    FILE *file = fopen(filename, "rb");  // 바이너리 모드로 파일을 엽니다
    if (file == NULL) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        return 0;  // 파일 열기 실패 시 0 반환
    }

    // 파일 포인터를 파일의 끝으로 이동
    fseek(file, 0, SEEK_END);
    
    // 파일 크기 얻기
    size_t file_size = ftell(file);
    
    // 파일 포인터를 원래 위치로 되돌림
    fseek(file, 0, SEEK_SET);
    
    fclose(file);
    
    return file_size;
}


// 에러 처리 함수
void handleErrors() {
    unsigned long err_code = ERR_get_error();
    if (err_code) {
        char err_msg[120];
        ERR_error_string_n(err_code, err_msg, sizeof(err_msg));
        printf("OpenSSL Error: %s\n", err_msg);
    } else {
        printf("Unknown error occurred.\n");
    }
    exit(1);
}

// 현재 시간 기반으로 기록용 타임스탬프 생성 함수
void generate_timestamp(char* timestamp, size_t size) {
    time_t now;
    struct tm *timeinfo;

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, size, "%Y-%m-%d %H:%M:%S", timeinfo); // 날짜와 시간을 "YYYY-MM-DD HH:MM:SS" 형식으로 생성
}

// 현재 시간 기반으로 파일 이름을 생성하는 함수 (경로 추가)
void generate_filename_with_timestamp(char* filename, size_t size, const char* prefix, const char* subdir) {
    time_t now;
    struct tm *timeinfo;
    char timestamp[20];

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H%M%S", timeinfo);

    snprintf(filename, size, "./data/%s/%s_%s.txt", subdir, prefix, timestamp);
}


// 현재 날짜와 시간을 기반으로 파일 이름 생성
void generate_filename(char *filename, size_t size) {
    time_t t;
    struct tm *tm_info;
    char time_str[20];

    // 현재 시간 가져오기
    time(&t);
    tm_info = localtime(&t);

    strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H%M%S", tm_info);
 
    snprintf(filename, size, "./data/encrypted_files/encrypted_%s.bin", time_str);
}

// 바이트 배열을 16진수로 출력하는 함수
void print_hex(unsigned char* data, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

void write_hex_to_file(unsigned char* data, size_t len, FILE* output_file) {
    for (size_t i = 0; i < len; i++) {
        fprintf(output_file, "%02x", data[i]);
        if ((i + 1) % 16 == 0)
            fprintf(output_file, "\n");
        else
            fprintf(output_file, " ");
    }
    fprintf(output_file, "\n");
}


size_t read_file(const char* filename, unsigned char* buffer, size_t max_len) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("파일 열기 오류");
        return 0;
    }
    size_t read_len = fread(buffer, 1, max_len, file);
    fclose(file);
    return read_len;
}

size_t write_file(const char* filename, const unsigned char* buffer, size_t len) {
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        perror("파일 쓰기 오류");
        return 0;
    }
    size_t written_len = fwrite(buffer, 1, len, file);
    fclose(file);
    return written_len;
}

