#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// 定义结构体
struct Data {
    int a;
    int b;
    char str[100];
};

int main() {
    const char *fifo_path = "/tmp/my_fifo";
    int fd;

    // 创建命名管道
    mkfifo(fifo_path, 0666);

    // 打开命名管道
    fd = open(fifo_path, O_WRONLY);

    struct Data data;
    data.a = 1;
    data.b = 2;
    strcpy(data.str, "Hello from C");

    // 发送1000个结构体
    int count = 0;
    while (count < 1000) {
        write(fd, &data, sizeof(data));
        printf("Written: %d %d %s\n", data.a, data.b, data.str);
        data.a++;
        data.b++;
        count++;
        sleep(1);
    }

    // 关闭管道
    close(fd);
    // 删除命名管道
    unlink(fifo_path);

    return 0;
}
