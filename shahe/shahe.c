#include "dr_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#define FIFO_NAME "/tmp/my_fifo"

typedef struct {
    int a;
    int b;
    char str[50];
} MyStruct;

void write_to_fifo(void *drcontext) {
    int fd;
    MyStruct data;
    char buffer[128];

    // 直接调用系统调用创建命名管道
    if (syscall(SYS_mkfifo, FIFO_NAME, 0666) == -1) {
        perror("mkfifo failed");
        return;
    }

    // 打开命名管道
    fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open failed");
        return;
    }

    // 不断写入数据到管道中
    for (int i = 0; i < 10; i++) {
        data.a = i;
        data.b = i * 2;
        snprintf(data.str, sizeof(data.str), "Message %d", i);

        // 写入结构体数据到管道中
        write(fd, &data, sizeof(MyStruct));

        sleep(1); // 休眠1秒钟
    }

    close(fd);
}

DR_EXPORT void dr_client_main(client_id_t id, int argc, const char *argv[]) {
    dr_set_client_name("DynamoRIO Sample Client", "http://dynamorio.org/issues");
    dr_register_exit_event(write_to_fifo);
}
