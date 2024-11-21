# OpenSSL 경로 설정
OPENSSL_DIR = /opt/homebrew/opt/openssl@3
SQLITE_DIR = /opt/homebrew/opt/sqlite3  # SQLite 경로 설정 (필요하다면 추가)

# 컴파일러 및 옵션 설정
CC = clang
CFLAGS = -Wall -g -I$(OPENSSL_DIR)/include -I./includes -DUSER_LITTLE_ENDIAN  # 엔디안 매크로 정의 추가
LDFLAGS = -L$(OPENSSL_DIR)/lib -lssl -lcrypto -lsqlite3

# aes_program 타겟 설정
AES_TARGET = aes_program
AES_OBJS = src/aes.o src/main.o src/utils.o src/sha.o

# user_management 타겟 설정
USER_TARGET = user_management
USER_OBJS = src/login.o src/sha.o  # 필요한 다른 오브젝트 파일들 추가

# aes_program 타겟 만들기
$(AES_TARGET): $(AES_OBJS)
	$(CC) $(CFLAGS) -o $(AES_TARGET) $(AES_OBJS) $(LDFLAGS)

# user_management 타겟 만들기
$(USER_TARGET): $(USER_OBJS)
	$(CC) $(CFLAGS) -o $(USER_TARGET) $(USER_OBJS) $(LDFLAGS)

# aes.o 파일 컴파일 규칙
src/aes.o: src/aes.c
	$(CC) $(CFLAGS) -c src/aes.c -o src/aes.o

# main.o 파일 컴파일 규칙
src/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o src/main.o

# sha.o 파일 컴파일 규칙
src/sha.o: src/sha.c
	$(CC) $(CFLAGS) -c src/sha.c -o src/sha.o

# login.o 파일 컴파일 규칙
src/login.o: src/login.c
	$(CC) $(CFLAGS) -c src/login.c -o src/login.o

# utils.o 파일 컴파일 규칙
src/utils.o: src/utils.c
	$(CC) $(CFLAGS) -c src/utils.c -o src/utils.o

# 클린 규칙: 오브젝트 파일 및 타겟 파일 삭제
clean:
	rm -f $(AES_OBJS) $(USER_OBJS) $(AES_TARGET) $(USER_TARGET)

# 기본 실행 규칙: make 명령어만 입력했을 때 실행
all: $(AES_TARGET) $(USER_TARGET)
