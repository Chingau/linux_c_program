/****************************************************************
file:         tcom_main.c
description:  the source file of thread communciation implementation
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>
#include "tcom_api.h"
#include "tcom.h"
#include "init.h"
#include "dev_rw.h"

static TCOM_PIPE  tcom_pipe[MPU_APP_MID_COUNT];
static TCOM_QUEUE tcom_queue[MPU_APP_MID_COUNT];
static pthread_mutex_t tcom_queue_mutex[MPU_APP_MID_COUNT];
static unsigned char tcom_msgbuf[TCOM_MAX_MSG_LEN];
static pthread_t tcom_tid;  /* thread id */

/***************************************************************
function:     tcom_push_data
description:  put msg data to queue buffer
input:        int queueidx, queue index in tcom_queue
              unsigned char *data, data will be pushed
              unsigned int datalen, data length
output:       none
return:       none
****************************************************************/
static void tcom_push_data(int queueidx, const unsigned char *data, unsigned int datalen)
{
    int part1_size;
    int part2_size;

    if (tcom_queue[queueidx].tail >= tcom_queue[queueidx].head)
    {
        if (TCOM_QUEUE_BUF_SIZE - tcom_queue[queueidx].tail >= datalen)
        {
            memcpy(tcom_queue[queueidx].buf + tcom_queue[queueidx].tail, data, datalen);
        }
        else
        {
            part1_size = TCOM_QUEUE_BUF_SIZE - tcom_queue[queueidx].tail;
            memcpy(tcom_queue[queueidx].buf + tcom_queue[queueidx].tail, data, part1_size);
            part2_size = datalen - part1_size;
            memcpy(tcom_queue[queueidx].buf, data + part1_size, part2_size);
        }
    }
    else
    {
        memcpy(tcom_queue[queueidx].buf + tcom_queue[queueidx].tail, data, datalen);
    }

    return;
}

/***************************************************************
function:     tcom_pull_data
description:  pull msg data from queue buffer
input:        int queueidx, queue index in tcom_queue
              unsigned int datalen, data length
output:       unsigned char *data, save the data to data buffer
return:       none
****************************************************************/
static void tcom_pull_data(int queueidx, unsigned char *data, unsigned int datalen)
{
    int part1_size;
    int part2_size;

    if (tcom_queue[queueidx].head >= tcom_queue[queueidx].tail)
    {
        if (TCOM_QUEUE_BUF_SIZE - tcom_queue[queueidx].head >= datalen)
        {
            memcpy(data, tcom_queue[queueidx].buf + tcom_queue[queueidx].head, datalen);
        }
        else
        {
            part1_size = TCOM_QUEUE_BUF_SIZE - tcom_queue[queueidx].head;
            memcpy(data, tcom_queue[queueidx].buf + tcom_queue[queueidx].head, part1_size);
            part2_size = datalen - part1_size;
            memcpy(data + part1_size, tcom_queue[queueidx].buf,  part2_size);
        }
    }
    else
    {
        memcpy(data, tcom_queue[queueidx].buf + tcom_queue[queueidx].head, datalen);
    }

    return;
}

/***************************************************************
function:     tcom_enqueue_msg
description:  put one msg to queue
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int tcom_enqueue_msg(int queueidx, const TCOM_MSG_HEADER *msghdr,
                            const unsigned char *msgbody)
{
    int idle_size;

    if (0 == msghdr->msglen)
    {
        return 0;
    }

    idle_size = (tcom_queue[queueidx].head - tcom_queue[queueidx].tail + TCOM_QUEUE_BUF_SIZE - 1) %
                TCOM_QUEUE_BUF_SIZE;

    /* the unused size is not enough */
    if (idle_size < msghdr->msglen)
    {
        printf("tcom_enqueue_msg queue overflow, sender:0x%04x, receiver:0x%04x, msgid:%u, length:%u\r\n",
              msghdr->sender, msghdr->receiver, msghdr->msgid, msghdr->msglen);
        return TCOM_QUEUE_OVERFLOW;
    }

    /* push msg body to queue */
    tcom_push_data(queueidx, msgbody, msghdr->msglen);
    tcom_queue[queueidx].tail = (tcom_queue[queueidx].tail + msghdr->msglen) % TCOM_QUEUE_BUF_SIZE;

    return 0;
}

/***************************************************************
function:     tcom_dequeue_msg
description:  get one msg from queue
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int tcom_dequeue_msg(int queueidx, TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    int used_size;

    if (0 == msghdr->msglen)
    {
        return 0;
    }

    used_size = (tcom_queue[queueidx].tail - tcom_queue[queueidx].head + TCOM_QUEUE_BUF_SIZE) %
                TCOM_QUEUE_BUF_SIZE;

    /* the unused size is not enough */
    if (used_size < msghdr->msglen)
    {
        printf("tcom_dequeue_msg queue underflow, sender:%u, receiver:%u, msgid:%u, length:%u\r\n",
              msghdr->sender, msghdr->receiver, msghdr->msgid, msghdr->msglen);
        return TCOM_QUEUE_UNDERFLOW;
    }

    /* push msg body to queue */
    tcom_pull_data(queueidx, msgbody, msghdr->msglen);
    tcom_queue[queueidx].head = (tcom_queue[queueidx].head + msghdr->msglen) % TCOM_QUEUE_BUF_SIZE;

    return 0;
}

/***************************************************************
function:     tcom_init_queue
description:  initiaze queue
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static void tcom_init_queue(int queueidx)
{
    /* init msg queue */
    tcom_queue[queueidx].head = 0;
    tcom_queue[queueidx].tail = 0;
}

/***************************************************************
function:     tcom_forward
description:  forward the message to the receiver directly
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static int tcom_forward(const TCOM_MSG_HEADER *msghdr, unsigned short receiver,
                        const unsigned char *msgbody)
{
    int index;
    int ret;
    unsigned char *buf;

    printf("tcom_forward, reciver:0x%04x\r\n", receiver);

    index = MID_TO_INDEX(receiver);

    pthread_mutex_lock(&tcom_queue_mutex[index]);

    if (0 != msghdr->msglen)
    {
        /* put message body into queue */
        ret = tcom_enqueue_msg(index, msghdr, msgbody);

        if (ret != 0)
        {
            printf("tcom_enqueue_msg failed, ret:0x%08x,sender:%u,receiver:%u,msgid:%u\r\n",
                  ret, msghdr->receiver, msghdr->sender, msghdr->msgid);
            pthread_mutex_unlock(&tcom_queue_mutex[index]);
            return -1;
        }
    }

    /* write the message header to pipe */
    buf = (unsigned char *)msghdr;
    ret = dev_write(tcom_pipe[index].fd[TCOM_PIPE_WRITE], buf, sizeof(TCOM_MSG_HEADER));

    if (ret != 0)
    {
        /* if write failed, rollback */
        printf("dev_write failed, ret:0x%08x,sender:%u,receiver:%u,msgid:%u\r\n",
              ret, msghdr->receiver, msghdr->sender, msghdr->msgid);

        if (msghdr->msglen > 0)
        {
            tcom_queue[index].tail = (tcom_queue[index].tail - msghdr->msglen
                                      + TCOM_QUEUE_BUF_SIZE) % TCOM_QUEUE_BUF_SIZE;
        }
    }

    pthread_mutex_unlock(&tcom_queue_mutex[index]);

    return ret;
}

/****************************************************************
function:     tcom_proc_msg
description:  read and process the message
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
static int tcom_proc_msg(void)
{
    int ret = 0;
    TCOM_MSG_HEADER msghdr;

    /* receive one message from thread communciation module */
    ret = tcom_recv_msg(MPU_MID_TCOM, &msghdr, tcom_msgbuf);

    if (ret != 0)
    {
        printf("tcom_recv_msg failed, ret:0x%08x\r\n", ret);
        return TCOM_RECV_MSG_FAILED;
    }

    //消息头中的接收者不能是MPU_MID_TCOM，否则会造成死循环(TCOM负责转发，自己转发给自己就会造成死循环)
    if (MPU_MID_TCOM != (msghdr.receiver & CPUID_MAIN_MID_MASK))
    {
        printf("forwardmsg, sender:0x%04x, receiver:0x%04x, msgid:0x%08x, msglen:0x%08x\r\n",
              msghdr.sender, msghdr.receiver, msghdr.msgid, msghdr.msglen);
        /* forward it*/
        ret = tcom_forward(&msghdr, msghdr.receiver, tcom_msgbuf);

        if (ret != 0)
        {
            printf("tcom_forward failed, ret:0x%08x\r\n", ret);
            return ret;
        }
    }
    else /* the message is sent to thread communciation moudle */
    {
        /* timer messge, do nothing */
        if (MPU_MID_TIMER == msghdr.sender)
        {

        }
    }

    return 0;
}

/****************************************************************
function:     tcom_main
description:  thread communciation module main function
input:        none
output:       none
return:       NULL
****************************************************************/
void *tcom_main(void)
{
    int index;
    int maxfd = 0;
    int ret;
    fd_set fds;

    index = MID_TO_INDEX(MPU_MID_TCOM);

    FD_ZERO(&fds);
    FD_SET(tcom_pipe[index].fd[TCOM_PIPE_READ], &fds);
    maxfd = tcom_pipe[index].fd[TCOM_PIPE_READ];

    while (1)
    {
        /* monitor the incoming data */
        ret = select(maxfd + 1, &fds, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if (ret > 0 && FD_ISSET(tcom_pipe[index].fd[TCOM_PIPE_READ], &fds))
        {
            tcom_proc_msg();
            continue;
        }
        else if (0 == ret)   /* timeout */
        {
            continue;   /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            printf("tcom_main exit, error:%s\r\n", strerror(errno));
            break;  /* thread exit abnormally */
        }
    }

    return NULL;
}

/****************************************************************
function:     tcom_sendmsg
description:  send message to other module
input:        TCOM_MSG_HEADER *msgheader, message header;
              unsigned char *msgbody, message body
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int tcom_send_msg(const TCOM_MSG_HEADER *msghdr, const void *msgbody)
{
    int ret;

    if (NULL == msghdr)
    {
        printf("msghdr is NULL\r\n");
        return TCOM_INVALID_PARAMETER;
    }

    if (((msghdr->sender & CPUID_MASK) != 0) || ((msghdr->receiver & CPUID_MASK) != 0))
    {
        printf("CPUID_MASK , sender or receiver is invalid,sender:0x%04x, receiver:0x%04x\r\n",
              msghdr->sender, msghdr->receiver);
        return TCOM_INVALID_PARAMETER;
    }

    if (((msghdr->sender & CPUID_MAIN_MID_MASK) > MPU_MAX_MID)
        || ((msghdr->receiver & CPUID_MAIN_MID_MASK) > MPU_MAX_MID))
    {
        printf("MPU_MAX_MID,sender or receiver is invalid,sender:0x%04x, receiver:0x%04x\r\n",
              msghdr->sender, msghdr->receiver);
        return TCOM_INVALID_PARAMETER;
    }

    if ((0 == (msghdr->sender & CPUID_MAIN_MID_MASK))
        || (0 == (msghdr->receiver & CPUID_MAIN_MID_MASK)))
    {
        printf("CPUID_MAIN_MID_MASK,sender or receiver is invalid,sender:0x%04x, receiver:0x%04x\r\n",
              msghdr->sender, msghdr->receiver);
        return TCOM_INVALID_PARAMETER;
    }

    if ((msghdr->msglen != 0) && (NULL == msgbody))
    {
        printf("msg header is invalid, sender:0x%04x, receiver:0x%04x, msgid:0x%08x\r\n",
              msghdr->sender, msghdr->receiver, msghdr->msgid);
        return TCOM_INVALID_PARAMETER;
    }

    if (msghdr->msglen > TCOM_MAX_MSG_LEN)
    {
        printf("msg length is invalid senderid:%04x,receiverid:%04x,msgid:%d\r\n", msghdr->sender,
              msghdr->receiver, msghdr->msgid);
        return TCOM_INVALID_PARAMETER;
    }

    /* forward the message to thread communciation module */
    ret = tcom_forward(msghdr, MPU_MID_TCOM, msgbody);

    if (ret != 0)
    {
        return ret;
    }

    return 0;
}


/****************************************************************
function:     tcom_get_read_fd
description:  get the read  file descriptor
input:        unsigned short receiver, the main module id of receiver;
output:       none
return:       positive value indicates the file descriptor;
              -1 indicates failed
*****************************************************************/
int tcom_get_read_fd(unsigned short main_mid)
{
    int index;

    if ((0 == (main_mid >> 8)) || ((main_mid & CPUID_MAIN_MID_MASK) > MPU_MAX_MID))
    {
        printf("main_mid(0x%08x) is invalid\r\n", main_mid);
        return TCOM_INVALID_PARAMETER;
    }

    /* this is ony used by thread communciation module */
    if (MPU_MID_TCOM == (main_mid & CPUID_MAIN_MID_MASK))
    {
        printf("TCOM fd can not be read\r\n");
        return TCOM_INVALID_PARAMETER;
    }

    index = MID_TO_INDEX(main_mid);

    return tcom_pipe[index].fd[TCOM_PIPE_READ];
}

/****************************************************************
function:     tcom_recv_msg
description:  receive the message from read file descriptor
input:        unsigned short receiver, the main module id of receiver;
output:       none
return:       0 indicates success;
              -1 indicates failed
*****************************************************************/
int tcom_recv_msg(unsigned short main_mid, TCOM_MSG_HEADER *msghdr, unsigned char *msgbody)
{
    int ret = 0;
    int index;
    unsigned char *buf;

    index = MID_TO_INDEX(main_mid);

    /* receive message header from pipe */
    pthread_mutex_lock(&tcom_queue_mutex[index]);

    buf = (unsigned char *)msghdr;
    ret = dev_read(tcom_pipe[index].fd[TCOM_PIPE_READ], buf, sizeof(TCOM_MSG_HEADER));

    if (ret != 0)
    {
        pthread_mutex_unlock(&tcom_queue_mutex[index]);
        return ret;
    }

    /* there is no msg body*/
    if (0 == msghdr->msglen)
    {
        pthread_mutex_unlock(&tcom_queue_mutex[index]);
        return 0;
    }

    if (NULL == msgbody)
    {
        printf("msgbody is NULL\r\n");
        pthread_mutex_unlock(&tcom_queue_mutex[index]);
        return TCOM_INVALID_PARAMETER;
    }

    ret = tcom_dequeue_msg(index, msghdr, msgbody);

    if (ret != 0)
    {
        tcom_init_queue(index);
        pthread_mutex_unlock(&tcom_queue_mutex[index]);
        return ret;
    }

    pthread_mutex_unlock(&tcom_queue_mutex[index]);

    return 0;
}

/****************************************************************
function:     tcom_run
description:  startup thread communciation module
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int tcom_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&tcom_tid, &ta, (void *)tcom_main, NULL);

    if (ret != 0)
    {
        printf("pthread_create failed, error:%s\r\n", strerror(errno));
        return TCOM_CREATE_THREAD_FAILED;
    }

    return 0;
}

/****************************************************************
function:     tcom_init
description:  initiaze thread communciation module
input:        unsigned char phase, the initization phase
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int tcom_init(int phase)
{
    int i, ret;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            for (i = 0; i < MPU_APP_MID_COUNT; i++)
            {
                /* create pipe */
                ret = pipe(tcom_pipe[i].fd);

                if (0 != ret)
                {
                    printf("pipe failed, error:%s\r\n", strerror(errno));
                    return TCOM_CREATE_PIPE_FAILED;
                }

                int flag;
                flag = fcntl(tcom_pipe[i].fd[TCOM_PIPE_WRITE], F_GETFL);
                ret  = fcntl(tcom_pipe[i].fd[TCOM_PIPE_WRITE], F_SETFL, flag | O_NONBLOCK);

                if (ret < 0)
                {
                    printf("fcntl failed, error:%s\r\n", strerror(errno));
                    return TCOM_SET_PIPE_FAILED;
                }

                /* init msg queue */
                tcom_init_queue(i);

                /* init mutex */
                pthread_mutex_init(&tcom_queue_mutex[i], NULL);
            }

            break;

        case INIT_PHASE_RESTORE:
            break;


        case INIT_PHASE_OUTSIDE:
            break;

        default:
            break;
    }

    return 0;
}
