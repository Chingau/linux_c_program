#include <stdio.h>
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include "init.h"
#include "timer_api.h"
#include "tcom_api.h"
#include "test_api.h"

static pthread_t test_tid;
static timer_t test_timer;
unsigned char buf[1024];

static void* test_main(void)
{
    int ret, max_fd = 0;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;

    max_fd = tcom_get_read_fd(MPU_MID_TEST);
    if (max_fd < 0) {
        printf("get test module fd failed\r\n");
        return NULL;
    }

    while (1) {
        FD_ZERO(&fds);
        FD_SET(max_fd, &fds);
        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        //the file descriptor is readable
        if (ret > 0) {
            if (FD_ISSET(max_fd, &fds)) {
                memset(buf, 0, sizeof(buf));
                if (tcom_recv_msg(MPU_MID_TEST, &msgheader, buf) == 0) {
                    /*
                    这里可以根据实现需要判断自己关心的内容，如下：
                    if (msgheader.sender == MPU_MID_TIMER) {

                    }

                    switch (msgheader.msgid) {
                        case xxxx:
                        break;
                        ...
                    }
                    */
                    printf("TEST MODULE recv from tcom msgheader: sender:0x%04x, receiver:0x%04x, msgid:0x%04x, msglen:0x%04x\r\n\n", 
                        msgheader.sender, msgheader.receiver, msgheader.msgid, msgheader.msglen);
                }
            }
        } else if (ret == 0) {
            //timeout
            continue;
        } else {
            if (errno == EINTR) {
                continue;
            }

            printf("test thread exit abnormally!\r\n");
            break;
        }
    }

    return 0;
}

int test_init(int phase)
{
    int ret;

    switch (phase) {
        case INIT_PHASE_INSIDE:
        //创建一个定时器，用于测试
        ret = tm_create(TIMER_REL, TEST_NAME_1, MPU_MID_TEST, &test_timer);
        if (ret != 0) {
            printf("tm_create test timer failed, ret:%0x08x\r\n", ret);
            return ret;
        }
        break;

        case INIT_PHASE_RESTORE:
        //添加自己的初始化内容
        break;

        case INIT_PHASE_OUTSIDE:
        tm_start(test_timer, 1000, TIMER_TIMEOUT_REL_PERIOD);
        break;

        default:
        break;
    }

    return 0;
}

int test_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&test_tid, &ta, (void *)test_main, NULL);

    if (ret != 0) {
        return -1;
    }

    return 0;
}
