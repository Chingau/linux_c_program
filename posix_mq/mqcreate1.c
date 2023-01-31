#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
int main(int argc, char *argv[])
{
    int ch, flags;
    mqd_t mqd;

    flags = O_RDWR | O_CREAT;
    while ( (ch = getopt(argc, argv, "e")) != -1) {
        switch (ch) {
            case 'e':
                flags |= O_EXCL;
                break;
            default:
                break;
        }
    }

    if (optind != argc - 1) {
        perror("usage: mqcreate [ -e ] <name>");
        exit(0);
    }

    if ( (mqd = mq_open(argv[1], flags, FILE_MODE, NULL)) < 0) {
        perror("mq_open error");
        exit(0);
    }

    mq_close(mqd);
    return 0;
}
