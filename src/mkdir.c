#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #define CREATE_FOLDER(folderName) _mkdir(folderName)
#else
    #include <sys/stat.h>
    #define CREATE_FOLDER(folderName) mkdir(folderName, 0755)
#endif

// 디렉토리 생성 함수
void create_folder(const char *path) {
    struct stat st;

    // 경로 존재 여부 확인
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            printf("디렉토리가 이미 존재합니다: %s\n", path);
        } else {
            printf("동일한 이름의 파일이 존재합니다: %s\n", path);
        }
    } else {
        if (mkdir(path, 0755) == 0) {
            printf("디렉토리 생성 성공: %s\n", path);
        } else {
            perror("디렉토리 생성 실패");
        }
    }
}

int main(void) {
    char username[100]; // 충분한 크기의 배열로 사용자 이름을 받기 위해 선언

    // 사용자 이름 입력 받기
    printf("Enter username: ");
    scanf("%s", username);

    // 사용자 디렉토리 생성
    char path[256];
    snprintf(path, sizeof(path), "data/userdata/%s", username);
    create_folder(path);

    return 0;
}
