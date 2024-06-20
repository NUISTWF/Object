#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    char *dataLen = NULL;
    char buffer[100] = {0};
    int len = 0;
    
    int data = 0;
    int led_fd = open("/dev/led", O_RDWR);
    if (led_fd < 0) {
		printf("open error");
        return 0;
	}

    printf("Content-type:text/html\n\n"); // 这句一定要加上
    //printf("hello");
    dataLen = getenv("CONTENT_LENGTH"); // 获取文本内容长度
    if (NULL == dataLen) {
        printf("error1");
        return 0;
    } else {
        len = atoi(dataLen);    // 先通过getenv获取长度再通过fgets和scanf一样的效果，获取内容
        if (len > 0) {
            if (NULL == fgets(buffer, len + 1, stdin)) { // buff == "open"
                printf("error2");
                return 0;
            } else {
                if (!strcmp(buffer, "open")) {
                    data = 1;
                    write(led_fd, &data, 4);
                    printf("open led");
                } else if (!strcmp(buffer, "close")) {
                    data = 0;
                    write(led_fd, &data, 4);
                    printf("close led");
                } else {
                    printf("error3");
                }
            }
        }
    }

    return 0;
}