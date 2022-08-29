# POSIX 定时器
posix 定时器是为了解决间隔定时器 itimer 存在的以下问题：

1. 一个进程同一时刻只能有一个同一种类型(ITIMER_REAL, ITIMER_PROF, ITIMER_VIRT)的 itimer。`posix定时器在一个进程中可以创建任意多个timer`。
2. itimer 定时器到期后只能通过信号(SIGALRM, SIGVTALRM, SIGPROF)的方式通知进程，posix定时器到期后不仅可以通过信号进行通知，`也可以使用自定义信号，还可以通过启动一个线程来进行通知`。
3. itimer 支持 us 级别，而 posix 定时器支持 ns 级别。

POSIX定时器提供的定时器 API 如下：

```c
int timer_create(clockid_t clock_id, struct sigevent *evp, timer_t *timerid)；
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value, struct itimerspect *ovalue);
int timer_gettime(timer_t timerid,struct itimerspec *value);
int timer_getoverrun(timer_t timerid);
int timer_delete (timer_t timerid);
```

## struct itimerspec 结构
其中时间结构体 itimerspec 和 itimer 的 itimerval 结构体用处及含义类似，只是提供了 ns 级别的精度；itimerspec 结构体定义如下：

```c
struct itimerspec {
    struct timespec it_interval;    //时间间隔
    struct timespec it_value;   //首次到期时间
};

struct timespec {
    time_t tv_sec;  //秒
    long tv_nsec;   //纳秒
};
```

`it_value`表示定时时间经过这么长时间到时，当定时器到时后，就会将`it_interval`的值赋给`it_value`。如果`it_interval`等于0，那么表示该定时器不是一个时间间隔定时器，一旦`it_value`到期后定时器就回到未启动状态。

## timer_create()
函数原型：

```c
int timer_create(clockid_t clock_id, struct sigevent *evp, timer_t *timerid);
```

功能：创建一个 posix timer；在创建的时候，需要指出定时器的类型，定时器超时通知机制。创建成功后通过参数返回创建的定时器的 ID。

`clock_id:` 参数 clock_id 用来指定定时器时钟的类型，时钟类型有以下6种：
|时钟类型|说明|
|:--|:--|
CLOCK_REALTIME|系统实时时间，即日历时间
CLOCK_MONOTONIC|从系统启动开始到现在为止的时间
CLOCK_PROCESS_CPUTIME_ID|本进程启动到执行到当前代码，系统CPU花费的时间
CLOCK_THREAD_CPUTIME_ID|本线程启动到执行到当前代码，系统CPU花费的时间
CLOCK_REALTIME_HR|CLOCK_REALTIME的细粒度(高精度)版本
CLOCK_MONOTONIC_HR|CLOCK_MONOTONIC的细粒度版本

`evp:` struct sigevent 设置了定时器到期时的通知方式和处理方式等，结构体定义如下：

```c
struct sigevent {
    int sigev_notify;   //设置定时器到期后的行为
    int sigev_signo;    //设置产生信号的信号码
    union sigval sigev_value;   //设置产生信号的值
    void (*sigev_notify_function)(union sigval);    //定时器到期，从该地址启动一个线程
    pthread_attr_t *sigev_notify_attributes;    //创建线程的属性
};

union sigval {
    int sival_int;
    void *sival_ptr;
}
```

如果 sigevent 传入 NULL，那么定时器到期会产生默认的信号，对 CLOCK_REALTIMER 来说，默认信号就是 SIGALRM，如果要产生除默认信号之外的其他信号，程序必须将 evp->sigev_signo 设置为期望的信号码。

如果几个定时器产生了同一个信号，处理程序可以用 sigev_value 来区分是哪个定时器产生了信号。要实现这种功能，程序必须在为信号安装处理程序时，使用 struct sigaction 的成员 sa_flags 中的标志符 SA_SIGINFO。

sigev_notify 的值可以取以下几种：
- SIGEV_NONE: 定时器到期后什么都不做，只提供通过 timer_gettime 和 timer_getoverrun 查询超时信息。
- SIGEV_SIGNAL: 定时器到期后，内核会将 sigev_signo 所指定的信号传送给进程，在信号处理程序中，si_value 会被设定为 sigev_value 的值。
- SIGEV_THREAD: 定时器到期后，内核会以 sigev_notification_attributes 为线程属性创建一个线程，线程的入口地址为 sigev_notify_function，传入 sigev_value 作为一个参数。

`timerid:` 创建的定时器，此为输出参数。

## timer_settime()
函数原型：

```c
int timer_settime(timer_t timerid, int flags, const struct itimerspec *value, struct itimerspect *ovalue);
```

创建 posix 定时器后，该定时器并没启动，需要通过 timer_settime() 接口设置定时器的到期时间和周期触发时间。

`flags:` 该参数标识到期时间是一个绝对时间还是一个相对时间。flags 可取值为 `TIMER_ABSTIME` 则 `value` 的值为一个`绝对时间`；否则，`value` 为一个`相对时间`。

```c
#define TIMER_ABSTIME   1
//所以我们设置flags的值为0时可以设置为一个相对时间
```

`itimerspec:` 设置的超时时间，一般设置方式如下：

```c
timer_t timer;
struct itimerspec ts;
/* set_timer为要设置的超时时间，单位为ms*/
ts.it_value.tv_sec  = set_timer / 1000;
ts.it_value.tv_nsec = (set_timer % 1000) * 1000 * 1000;
timer_settime(timer, TIMER_ABSTIME, &ts, NULL); /* start absolute timer */
```

如果想要设置周期性触发的时间，那需要设置 ts.it_interval 的值，一般设置方式如下：

```c
timer_t timer;
struct itimerspec ts;
/* set_timer为要设置的超时时间，单位为ms*/
ts.it_value.tv_sec  = set_timer / 1000;
ts.it_value.tv_nsec = (set_timer % 1000) * 1000 * 1000;

/* 用于设置周期性触发 */
ts.it_interval.tv_sec  = ts.it_value.tv_sec;
ts.it_interval.tv_nsec = ts.it_value.tv_nsec;

timer_settime(timer, TIMER_ABSTIME, &ts, NULL); /* start absolute timer */
```

## timer_getoverrun()
函数原型：

```c
int timer_getoverrun(timer_t timerid);
```

取得一个定时器的超限运行次数：有可能一个定时器到期了，而同一定时器上一次到期时产生的信号还处于挂起状态。在这种情况下，其中的一个信号可能会丢失。这就是定时器超限。程序可以通过调用 `timer_getoverrun()` 来确定一个特定的定时器出现这种超限的次数。定时器超限只能发生在同一个定时器产生的信号上。由于多个定时器，甚至是那些使用相同的时钟和信号的定时器，所产生的信号都会排队而不会丢失。

执行成功时，`timer_getoverrun()` 会返回定时器初次到期与通知进程(例如通过信号)定时器已到期之间额外发生的定时器到期次数。举例来说，在我们之前的例子中，一个 1ms 的定时器运行了 10ms，则此调用会返回 9。如果超限运行的次数等于或大于 DELAYTIMER_MAX，则此调用会返回 DELAYTIMER_MAX。

执行失败时，此函数会返回 -1，并将 errno 设定为 EINVAL，这个唯一的错误情况代表 timerid 指定了无效的定时器。

## timer_delete()
函数原型：

```c
int timer_delete (timer_t timerid);
```

删除一个定时器：一次成功的 timer_delete() 调用会销毁关联到 timerid 的定时器并返回0。执行失败时，此调用会返回-1并将errno设定为 EINVAL，这个唯一的错误情况代表 timerid 不是一个有效的定时器。

posix 定时器通过调用内核的 posix_timer 进行实现，但 glibc 对 posix 进行了一定的封装，例如，如果 posix timer 到期通知方式被设置为 SIGEV_THREAD 时，glibc 需要自己完成一些辅助工作，因为内核无法在 timer 到期时启动一上新的线程。

```c
int timer_create (clock_id, evp, timerid)
     clockid_t clock_id;
     struct sigevent *evp;
     timer_t *timerid;
{
    if (evp == NULL || __builtin_expect (evp->sigev_notify != SIGEV_THREAD, 1))
    {
        ...
    }
    else
    {
          ...
          /* Create the helper thread.  */
          pthread_once (&__helper_once, __start_helper_thread);
          ...
    }
    ...
}
```

可以看到 glibc 发现用户需要启动新线程通知时，会自动调用 pthread_once 启动一个辅助线程(__start_helper_thread)，用 sigev_notify_attributes 中指定的属性设置该辅助线程。

然后 glibc 启动一个普通的 POSIX Timer，将其通知方式设置为：SIGEV_SIGNAL | SIGEV_THREAD_ID。这样就可以保证内核在 timer 到期时通知辅助线程。通知的 Signal 号为 SIGTIMER，并且携带一个包含了到期函数指针的数据。这样，当该辅助 Timer 到期时，内核会通过 SIGTIMER 通知辅助线程，辅助线程可以在信号携带的数据中得到用户设定的到期处理函数指针，利用该指针，辅助线程调用 pthread_create() 创建一个新的线程来调用该处理函数。这样就实现了 POSIX 的定义。