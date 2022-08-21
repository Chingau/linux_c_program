#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        perror("usage: mqunlink <name>");
        exit(0);
    }

    //移除一个消息队列
    if ( (mq_unlink(argv[1])) < 0) {
        perror("mq_unlink error");
        exit(0);
    }

    return 0;
}