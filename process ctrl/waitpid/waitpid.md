当用 fork 启动一个新的子进程的时候，子进程就有了新的生命周期，并将在其自己的地址空间内独立运行。但有时候，我们希望知道某一个自己创建的子进程何时结束，从而方便父进程做一些处理动作。同样的，在用 ptrace 去 attach 一个进程之后，那个被 attach 的进程某种意义上说可以算作那个 attach 它进程的子进程，这种情况下，有时候就想知道被调试的进程何时停止运行。

以上两种情况下，都可以使用 Linux 中的 `waitpid` 函数做到。先来看一下 waitpid 原型：

```c
#include <sys/wait.h>
#include <sys/types.h>

pid_t waitpid(pid_t pid, int *status, int options);
```

如果在调用 `waitpid` 函数时，当指定等待的子进程已经停止运行或结束了，则 `waitpid` 会立即返回；但是如果子进程还没有停止运行或结束，则调用 `waitpid` 函数的父进程则会被阻塞，暂停运行。

下面看一下 `waitpid` 函数的参数含义：

**pid**:参数 pid 为欲等待的子进程的 pid，其具体含义如下：
|参数值 |说明|
|:--|:--
pid < -1 | 等待进程组ID为 pid 绝对值的任一子进程
pid = -1 | 等待任一子进程，此时的waitpid()函数就退化成了普通的wait()函数
pid = 0 |等待组ID等待调用进程组ID的任一子进程，也就是说任何和调用waitpid()函数的进程在同一个进程组的进程
pid > 0| 等待进程ID与pid相等的子进程

**status**:这个参数将保存子进程的状态信息，有了这个信息父进程就可以了解子进程为什么会退出，是正常退出还是出了什么错误。如果 status 不是空指针，则状态信息将被写入其指向的位置。当然，如果不关心子进程为什么退出的话，也可以传入空指针。Linux 提供了一些非常有用的宏来帮助解析这个状态信息，这些宏定义在 `sys/wait.h` 头文件中。主要有以下几个：

|宏 | 说明|
|:--|:--
WIFEXITED(status)|如果子进程正常结束，它就返回真；否则返回假
WEXITSTATUS(status)|如果WIFEXITED(status)为真，则可以用该宏取得子进程exit()返回的结束码
WIFSIGNALED(status)|如果子进程因为一个未捕获的信号而终止，它就返回真;否则返回假
WTERMSIG(status)|如果WIFSIGNALED(status)为真，则可以用该宏获得导致子进程终止的信号码
WIFSTOPPED(status)|如果当前子进程被暂停了，则返回真；否则返回假
WSTOPSIG(status)|如果WIFSTOPPED(status)为真，则可以使用该宏获得导致子进程暂停的信号码

**options**:参数 options 提供了一些另外的选项来控制 waitpid 函数的行为。如果不想使用这些选项，则可以把这个参数设为 0。主要使用的有以下两个选项：

|参数 | 说明
|:--|:--
WNOHANG|如果 pid 指定的子进程没有结束，则 waitpid() 函数立即返回 0，而不是阻塞在这个函数上等待；如果结束了，则返回该子进程的进程号
WUNTRACED|如果子进程进入暂停状态，则马上返回

这些参数可以用 "|" 运算符连接起来使用。
如果 waitpid() 函数执行成功，则返回子进程的进程号；如果有错误发生，则返回 -1；并且将失败的原因存放在 errno 变量中。

失败的原因主要有：没有子进程(errno 设置为 ECHILD); 调用被某个信号中断(errno 设置为 EINTR) 或选项参数无效(errno 设置为 EINVAL)。

下面这种调用 waitpid() 函数等效于调用 wait()：

```c
waitpid(-1, status, 0);
```