[TOC]

在Linux中有许多进行`进程间通信`的方法。本篇主要学习一种常见的进程间通信的方法——Unix域套接字。

# 1. 简介
Unix 域套接字是一种在本机的进程间进行通信的一种方法。虽然 Unix 域套接字的接口与 TCP 和 UDP 套接字的接口十分相似，但是 Unix 域套接字只能用于同一台机器的进程间通信，不能让两个位于不同机器的进程进行通信。正是由于这个特性，Unix 域套接字可以可靠地在两个进程间复制数据，不用像 TCP 一样采用一些诸如"添加网络报头"、"计算检验和"、"产生顺序号"等一系列保证数据完整性的操作。因此，在同一台机器上进行进程间通信时，Unix 域套接字的效率往往比 TCP 套接字的效率更高。

# 2. Unix 域套按字地址结构
在使用 TCP 套接字和 UDP 套接字时，我们需要用 `struct sockaddr_in`(IPv4)定义套接字的地址结构，与之相似，Unix 域套接字使用`struct sockaddr_un`定义套接字的地址结构。

`struct sockaddr_un`的结构如下：（位于头文件 sys/un.h 中）

```c
struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[108];
}
```

在使用 internet 套接字进行编程时，需要将 `struct sockaddr_in` 的 `sin_samily` 成员设置成 AF_INET（IPv4）。与之类似，在使用 Unix 域套接字时，需要将 `sun_family` 成员设置成 `AF_UNIX` 或 `AF_LOCAL` (**这两个宏的作用完全相同，都表示 Unix 域**)。

`struct sockaddr_un` 的第二个成员 `sun_path` 表示 socket 的地址。在 Unix 域中，socket 的地址用路径名表示。例如，可以将 sun_path 设置为 `/tmp/unixsock`。由于路径名是一个字符串，所以 sun_path 必须能够容纳字符串的结尾 '\0'。需要注意的是，标准并没有规定 sun_path 的大小，在某些平台中，sun_path 的大小可能是 104,92 等值。所以如果需要保证可移植性，在编码时应该使用 sun_path 的最小值。

# 3. 创建 Unix 域套接字
Unix 域套接字使用 socket 函数创建，与 internet 套接字一样，Unix 域套接字也有流套接字和数据报套接字两种：

```c
int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0); //Unix 域中的流 socket
int sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);  //Unix 域中的数据报 socket
```

## 3.1 Unix 域中的流 socket
Unix 域中的流套接字与 TCP 流套接字的用法十分相似。在服务器端，我们首先创建一个 Unix 域流套接字，将其绑定到一个路径上，然后调用 `listen` 监听客户端连接，调用 `accept` 接受客户端的连接。在客户端，再创建一个 Unix 域流套接字之后，可以使用 `connect` 尝试连接指定的服务器套接字。这里有一个例子，是一个使用 Unix 域流套接字实现的 echo 服务器和客户端的程序。(源文件见 echo_server.c 及 echo_client.c)


## 3.2 Unix 域中的数据报 socket
Unix 域数据报套接字与 UDP 套接字类似，可以通过 Unix 域数据报套接字在进程间发送具有边界的数据报。但由于 Unix 域数据报套接字是在本机上进行通信，所以 Unix 域数据报套接字的数据传递是可靠的，不会像 UDP 套接字那样发生丢包的问题。Unix 域数据报套接字的接口与 UDP 也十分相似。在服务器端通常先创建一个 Unix 域数据报套接字，然后将其绑定到一个路径上，然后调用 `recvfrom` 接收客户端发送过来的数据，调用 `sendto` 向客户端发送数据。对于客户端，通常是先创建一个 Unix 域数据报套接字，将这个套接字绑定到一个路径上，然后调用 `sendto` 发送数据，调用 `recvfrom` 接收客户端发来的数据。这里有一个使用 Unix 域数据报实现的 echo 程序。（源文件见 echo_server_1.c 及 echo_client_1.c）

# 4. 绑定 Unix 域套接字
使用 `bind` 函数可以将一个 Unix 套接字绑定到一个地址上。绑定 Unix 域套接字时，bind 会在指定的路径名处创建一个表示 Unix 域套接字的文件。Unix 域套接字与路径名是一一对应关系，即一个 Unix 域套接字只能绑定一个路径名上。一般要把 Unix 域套接字绑定到一个绝对路径上，例如：

```c
struct sockaddr_un addr;
addr.sin_family = AF_LOCAL;
strcpy(addr.sun_path, "/tmp/sockaddr");  //指明路径

int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
if ( bind(sock_fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
    fprintf(stderr, "bind error\n");
}
```

Unix 域套接字被绑定后，可以使用 getsockname 获取套接字绑定的路径名：

```c
struct sockaddr_un addr;
socklen_t len = sizeof(addr);

getsockname(sock_fd, (const struct sockaddr *)&addr, &len);
printf("%s\n", addr.sun_path);
```

当一个套接字不再使用时应调用 `unlink` 将其删除。

# 5. Unix 域套接字的权限
当程序调用 `bind` 时，会在文件系统中的指定路径处创建一个与套接字对应的文件。我们可以通过控制该文件的权限来控制进程对这个套接字的访问。当进程想要连接一个 Unix 域套接字或通过一个 Unix 域数据报套接字发送数据时，需要拥有对该套接字的`写权限`以及对 socket 路径名的所有目录的`执行权限`。在调用`bind`时，会自动赋予用户、组和其他用户的所有权限。如果想要修改这一行为，可以在调用`bind`之前调用`umask`禁用掉某些权限。

# 6. 使用 socketpair 创建互联的 socket 对
有时我们需要在同一进程中创建一对相互连接的 Unix 域 socket (**类似于管道**)，这可以通过 `socket, bind, listen, accept, connect` 等调用实现。而 `socketpair` 提供了一个简单方便的方法来创建一对互联的 socket。`socketpair` 创建的一对 socket 是 `全双工` 的。socketpair 函数原型如下：

```c
#include <sys/socket.h>
int socketpair(int domain, int type, int protocol, int socketfd[2]);
```

socketpair 的前三个参数与 socket 函数的参数含义相同。由于 socketpair 只能用于 Unix 域套接字，所以 `domain` 参数必须是 `AF_UNIX` 或 `AF_LOCAL`；`type` 参数可以是 `SOCK_DGRAM` 或 `SOCK_STREAM`，分别创建一对数据报或流 socket；`protocol` 参数必须是 0; `socketfd` 用于返回创建的两个套接字文件描述符。

通常，在调用 socketpair 创建一对套接字后会调用 fork 创建子进程，这样父进程和子进程就可以通过这一对套接字进行进程间通信了。

# 7. 使用 Unix 域套接字传递描述符
Unix 域套接字的一个"特色功能"就是在进程间`传递描述符`。描述符可以通过 Unix 域套接字在没有亲缘关系的进程间传递。描述符是一种`辅助数据`，可以通过`sendmsg`发送，通过`recvmsg`接收。这里的`描述符`可以是`open, pipe, mkfifo, socket, accept`等函数打开的描述符。这里有一个例子子进程向父进程传递描述符(见源文件 fd_trans.c)。