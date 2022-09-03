#ifndef __TIMER_API_H__
#define __TIMER_API_H__
#include "moduleid_def.h"

typedef enum TIMER_TYPE
{
    TIMER_REL,     /* relative timer */
    TIMER_ABS,      /* absolute timer */
    TIMER_RTC = 8,  /* rtc timer defined by quectel sdk */
} TIMER_TYPE;


typedef enum TIMER_ERROR_CODE
{
    TIMER_INVALID_PARAMETER  = (MPU_MID_TIMER << 16) | 0x01,
    TIMER_CREATE_TIMER_FAILED,
    TIMER_START_TIMER_FAILED,
    TIMER_STOP_TIMER_FAILED,
    TIMER_DESTORY_TIMER_FAILED,
    TIMER_SET_TIME_FAILED,
} TIMER_ERROR_CODE;

typedef enum TIMER_TIMEOUT_TYPE
{
    TIMER_TIMEOUT_REL_ONCE,    /* one time relative timer */
    TIMER_TIMEOUT_REL_PERIOD,  /* period relative timer */
    TIMER_TIMEOUT_ABS_ONCE,    /* one time absolute timer */
} TIMER_TIMEOUT_TYPE;

int tm_create(TIMER_TYPE type, unsigned short timername, unsigned short mid, timer_t *timer);
int tm_start(timer_t timer, unsigned int interval, TIMER_TIMEOUT_TYPE type);
int tm_stop(timer_t timer);
int tm_destory(timer_t timer);
int tm_get_timer_state(timer_t timer);

#endif
