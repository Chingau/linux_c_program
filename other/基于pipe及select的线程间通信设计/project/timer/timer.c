/****************************************************************
file:         timer.c
description:  the source file of timer implementation
****************************************************************/

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "timer_api.h"
#include "tcom_api.h"

/*********************************************
function:     timer_timeout
description:  timeout callback
input:        union sigval value, the high 16 bytes is timername,
              and low 16 bytes is mid
output:       none
return:       none
*********************************************/
static void tm_timerout_handler(union sigval value)
{
    TCOM_MSG_HEADER msghdr;
    unsigned long long time;
    struct timespec now = {0, 0};

    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
    {
        printf("get system monotonic clock fail\r\n");
        return;
    }
    time = now.tv_sec;
    time = time * 1000000 + now.tv_nsec / 1000;
    msghdr.sender   = MPU_MID_TIMER;
    msghdr.receiver = value.sival_int >> 16;
    msghdr.msgid    = value.sival_int & 0xffff;
    msghdr.msglen   = sizeof(time);

    if (tcom_send_msg(&msghdr, &time) != 0)
    {
        printf("send message (0x%04x) to moudle(0x%04x) fail\r\n", msghdr.msgid, msghdr.receiver);
    }

    return;
}

/*********************************************
function:     tm_get_timer_state
description:  get the state of the timer
input:        timer_t timer, timer id;
output:       none
return:       0 the timer is inactive,
              1 the timer is active
*********************************************/
int tm_get_timer_state(timer_t timer)
{
    struct itimerspec timerspec;
    int ret;

    memset(&timerspec, 0, sizeof(timerspec));
    ret = timer_gettime(timer, &timerspec);
    if (ret == 0) {
        if (timerspec.it_value.tv_sec == 0 && timerspec.it_value.tv_nsec == 0) {
            ret = 0;
        } else {
            ret = 1;
        }
    }

    return ret;
}

/*********************************************
function:     timer_create
description:  create timer
input:        TIMER_TYPE type, timer type;
              unsigned short timerid, timername;
              unsigned short mid, moudle id;
output:       timer_t *timer, timer id
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_create(TIMER_TYPE type, unsigned short timername, unsigned short mid, timer_t *timer)
{
    int ret;
    struct sigevent evp;

    if (timer == NULL) {
        printf("timer parameter is NULL\r\n");
        return TIMER_INVALID_PARAMETER;
    }

    if (type != TIMER_REL && type != TIMER_ABS && type != TIMER_RTC) {
        printf("type parameter is invalid, type:%u\r\n", type);
        return TIMER_INVALID_PARAMETER;
    }

    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = tm_timerout_handler;
    evp.sigev_value.sival_int = timername | (mid << 16);
    evp.sigev_notify_attributes = NULL;

    switch (type) {
        case TIMER_REL:
        ret = timer_create(CLOCK_MONOTONIC, &evp, timer);
        break;

        case TIMER_ABS:
        ret = timer_create(CLOCK_REALTIME, &evp, timer);
        break;

        case TIMER_RTC:
        ret = timer_create(type, &evp, timer);
        break;

        default:
        break;
    }

    if (ret != 0) {
        printf("create timer (0x%04x) in module(0x%04x) failed, ret:%s", timername, mid, strerror(errno));
        return TIMER_CREATE_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     timer_start
description:  start timer
input:        timer_t timer, timer id;
              unsigned short interval, timer interval(the unit is ms);
              REL_TIMER_TYPE type, type of relative timer;
output:       none
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_start(timer_t timer, unsigned int interval, TIMER_TIMEOUT_TYPE type)
{
    struct itimerspec ts;
    struct timespec now;
    int ret;

    if (interval == 0) {
        printf("interval parameter is 0\r\n");
        return TIMER_INVALID_PARAMETER;
    }

    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    if (type == TIMER_TIMEOUT_REL_ONCE || type == TIMER_TIMEOUT_REL_PERIOD) {
        //设置首次超时时间，单位ms
        ts.it_value.tv_sec = interval/1000;
        ts.it_value.tv_nsec = (interval%1000)*1000*1000;

        if (type == TIMER_TIMEOUT_REL_PERIOD) {
            //设置周期性超时时间
            ts.it_interval.tv_sec = ts.it_value.tv_sec;
            ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
        }

        ret = timer_settime(timer, 0, &ts, NULL); //start relative timer
    } else {
        clock_gettime(CLOCK_REALTIME, &now);
        ts.it_value.tv_sec = (now.tv_sec + interval/1000) + ((interval%1000)*1000*1000 + now.tv_nsec)/1000000000UL;
        ts.it_value.tv_nsec = ((interval%1000)*1000*1000 + now.tv_nsec)/1000000000UL;

        ret = timer_settime(timer, TIMER_ABSTIME, &ts, NULL); //start absolute timer
    }

    if (ret != 0) {
        printf("start timer with interval(0x%04x) failed, ret:%s\r\n", interval, strerror(errno));
        return TIMER_START_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     timer_stop
description:  stop timer
input:        timer_t timer, timer id;
output:       none
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_stop(timer_t timer)
{
    struct itimerspec ts;
    int ret;

    memset(&ts, 0, sizeof(ts));
    ret = timer_settime(timer, 0, &ts, NULL);

    if (ret != 0) {
        printf("stop timer failed, ret:%s\r\n", strerror(errno));
        return TIMER_STOP_TIMER_FAILED;
    }

    return 0;
}

/*********************************************
function:     timer_destory
description:  destory timer, the timer will be deleted
input:        timer_t timer, timer id;
output:       none
return:       0 indicates success,
              others indicates failed
*********************************************/
int tm_destory(timer_t timer)
{
    int ret;

    ret = timer_delete(timer);

    if (ret != 0) {
        printf("destory timer failed, ret:%s\r\n", strerror(errno));
        return TIMER_DESTORY_TIMER_FAILED;
    }

    return 0;
}
