# OpenSSL 경로 설정
OPENSSL_DIR = /opt/homebrew/opt/openssl@3

# 컴파일러 및 옵션 설정
CC = clang
CFLAGS = -Wall -g -I$(OPENSSL_DIR)/include -I./includes -DUSER_LITTLE_ENDIAN  # 엔디안 매크로 정의 추가
LDFLAGS = -L$(OPENSSL_DIR)/lib -lssl -lcrypto

# 타겟 프로그램 이름
TARGET = aes_program

# 오브젝트 파일 목록
OBJS = src/aes.o src/main.o src/utils.o src/sha.o src/KISA_SHA256.o

# 기본 타겟: aes_program 만들기
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# aes.o 파일 컴파일 규칙
src/aes.o: src/aes.c
	$(CC) $(CFLAGS) -c src/aes.c -o src/aes.o

# main.o 파일 컴파일 규칙
src/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o src/main.o

# sha.o 파일 컴파일 규칙
src/sha.o: src/sha.c
	$(CC) $(CFLAGS) -c src/sha.c -o src/sha.o

# KISA_SHA256.o 파일 컴파일 규칙
src/KISA_SHA256.o: src/KISA_SHA256.c
	$(CC) $(CFLAGS) -c src/KISA_SHA256.c -o src/KISA_SHA256.o

# utils.o 파일 컴파일 규칙
src/utils.o: src/utils.c
	$(CC) $(CFLAGS) -c src/utils.c -o src/utils.o

# 클린 규칙: 오브젝트 파일 및 타겟 파일 삭제
clean:
	rm -f $(OBJS) $(TARGET)

# 기본 실행 규칙: make 명령어만 입력했을 때 실행
all: $(TARGET)
