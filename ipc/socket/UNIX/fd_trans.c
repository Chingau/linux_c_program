#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE  1
#define TEXT_SIZE 12

void sendFd(int fd, int socket_fd);
int  recvFd(int socket_fd);

int main(void)
{
    int fd_pair[2] = {0};

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fd_pair) < 0)
    {
        fprintf(stderr, "socket error\n");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "fork error\n");
        return -1;
    }
    if (pid > 0)
    {
        close(fd_pair[1]);
        int  recv_fd = recvFd(fd_pair[0]);
        char text[TEXT_SIZE + 1] = {0};
        if (read(recv_fd, text, TEXT_SIZE) != TEXT_SIZE)
        {
            fprintf(stderr, "read error\n");
            return -1;
        }
        printf("%s", text);
        if (waitpid(pid, NULL, 0) < 0)
        {
            fprintf(stderr, "waitpid error\n");
            return -1;
        }
        return 0;
    }

    close(fd_pair[0]);
    // ./hello.txt的内容为"hello world\n"
    int fd = open("./hello.txt", O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "open error\n");
        exit(EXIT_FAILURE);    // NOLINT
    }

    sendFd(fd, fd_pair[1]);

    return 0;
}

void sendFd(int fd, int socket_fd)
{
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    struct iovec iov;

    union
    {
        struct cmsghdr cm;
        char           control[CMSG_SPACE(sizeof(int))];
    } control_un;

    struct cmsghdr *cmptr      = NULL;
    msg.msg_control            = control_un.control;
    msg.msg_controllen         = sizeof(control_un.control);
    cmptr                      = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len            = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level          = SOL_SOCKET;
    cmptr->cmsg_type           = SCM_RIGHTS;
    *((int *)CMSG_DATA(cmptr)) = fd;
    msg.msg_name               = NULL;
    msg.msg_namelen            = 0;
    char buf[BUF_SIZE]         = {0};
    iov.iov_base               = &buf;
    iov.iov_len                = BUF_SIZE;
    msg.msg_iov                = &iov;
    msg.msg_iovlen             = 1;

    if (sendmsg(socket_fd, &msg, 0) < 0)
    {
        fprintf(stderr, "sendmsg error\n");
        exit(EXIT_FAILURE);  
    }
}

int recvFd(int socket_fd)
{
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));

    char buf[BUF_SIZE] = {0};
    struct iovec iov;
    iov.iov_base = buf;
    iov.iov_len  = BUF_SIZE;

    union
    {
        struct cmsghdr cm;
        char           control[CMSG_SPACE(sizeof(int))];
    } control_un;

    msg.msg_control       = control_un.control;
    msg.msg_controllen    = sizeof(control_un.control);
    msg.msg_name          = NULL;
    msg.msg_namelen       = 0;
    msg.msg_iov           = &iov;
    msg.msg_iovlen        = 1;
    struct cmsghdr *cmptr = CMSG_FIRSTHDR(&msg);

    if (recvmsg(socket_fd, &msg, 0) < 0)
    {
        fprintf(stderr, "recvmsg error\n");
        exit(EXIT_FAILURE);    // NOLINT
    }

    int fd = *((int *)CMSG_DATA(cmptr));
    return fd;
}
