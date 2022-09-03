#ifndef __TCOM_H__
#define __TCOM_H__

#define TCOM_QUEUE_BUF_SIZE (100*1024)

#define TCOM_PIPE_READ  0
#define TCOM_PIPE_WRITE 1

typedef struct TCOM_PIPE {
    int fd[2];
} TCOM_PIPE;

typedef struct TCOM_QUEUE {
    int head;
    int tail;
    unsigned char buf[TCOM_QUEUE_BUF_SIZE];
} TCOM_QUEUE;

#endif
