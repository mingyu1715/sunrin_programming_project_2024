# 1. 베이스 이미지 선택
FROM ubuntu:22.04

# 2. 필수 패키지 설치
RUN apt-get update && apt-get install -y \
    net-tools \
    build-essential \
    clang \
    libssl-dev \
    libsqlite3-dev \
    python3 \
    python3-pip \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# 3. 환경 변수 설정 (OpenSSL 및 SQLite 경로)
ENV OPENSSL_DIR=/usr/lib/ssl
ENV SQLITE_DIR=/usr/lib/sqlite3

# 4. 작업 디렉토리 설정
WORKDIR /app

# 5. 소스 파일 및 Makefile 복사
COPY . .

# 6. C 프로그램 빌드
RUN make all

# 7. Python 의존성 설치
WORKDIR /app/website
RUN pip3 install --no-cache-dir -r requirements.txt

# 8. Flask 애플리케이션 실행
CMD python3 app.py