#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

// 에러 처리, 헥스 출력 및 파일 입출력 함수 선언
void handleErrors();
void generate_filename(char *filename, size_t size);
size_t get_file_size(const char *filename);
void generate_timestamp(char* timestamp, size_t size);
void generate_filename_with_timestamp(char* filename, size_t size, const char* prefix, const char* subdir);
void print_hex(unsigned char* data, int len);
void write_hex_to_file(unsigned char* data, size_t len, FILE* output_file);
size_t read_file(const char* filename, unsigned char* buffer, size_t max_len);
size_t write_file(const char* filename, const unsigned char* buffer, size_t len);

#endif
