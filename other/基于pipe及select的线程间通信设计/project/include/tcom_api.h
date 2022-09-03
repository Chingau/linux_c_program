#ifndef __TCOM_API_H__
#define __TCOM_API_H__
#include "moduleid_def.h"

#define TCOM_MAX_MSG_LEN    (2*1024)

//消息头
typedef struct TCOM_MSG_HEADER
{
    unsigned short sender;    /* the mid of sender */
    unsigned short receiver;  /* the mid of receiver */
    unsigned int   msgid;     /* message id */
    unsigned int   msglen;    /* message body length */
} TCOM_MSG_HEADER;

//错误码
typedef enum TCOM_ERROR_CODE
{
    TCOM_INVALID_PARAMETER = (MPU_MID_TCOM << 16) & 0x01,
    TCOM_QUEUE_OVERFLOW,
    TCOM_QUEUE_UNDERFLOW,
    TCOM_RECV_MSG_FAILED,
    TCOM_CREATE_THREAD_FAILED,
    TCOM_CREATE_PIPE_FAILED,
    TCOM_SET_PIPE_FAILED,
} TCOM_ERROR_CODE;

#define TCOM_INIT_MSG_HEADER(header, _sender, _receiver, _msgid, _msglen) \
    do {                                \
        header.sender   = (_sender);    \
        header.receiver = (_receiver);  \
        header.msgid    = (_msgid);     \
        header.msglen   = (_msglen);    \
    } while(0)

/* initiaze thread communciation module */
int tcom_init(int phase);
/* startup thread communciation module */
int tcom_run(void);
/* send message to other module */
int tcom_send_msg(const TCOM_MSG_HEADER *msghdr, const void *msgbody);
/* get the read  file descriptor of the specified main module */
int tcom_get_read_fd(unsigned short main_mid);
/* receive one message for the specified main module */
int tcom_recv_msg(unsigned short main_mid, TCOM_MSG_HEADER *msghdr, unsigned char *msgbody);

#endif
