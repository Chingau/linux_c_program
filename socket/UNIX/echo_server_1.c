// 服务器
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_SOCKET_PATH "/tmp/echo_unix_socket"
#define MSG_MAX_LENGTH   100

void signalHandler(int signo)    
{
    fprintf(stdout, "socket exit!\n");
    unlink(UNIX_SOCKET_PATH);  
    exit(EXIT_SUCCESS);        
}

int main(void)
{
    if (signal(SIGINT, signalHandler) == SIG_ERR) 
    {
        fprintf(stderr, "signal error\n");
        return -1;
    }

    int listen_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (listen_fd < 0)
    {
        fprintf(stderr, "socket error\n");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, UNIX_SOCKET_PATH);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "bind error\n");
        return -1;
    }

    char buf[MSG_MAX_LENGTH + 1] = {0};
    for (;;)
    {
        struct sockaddr_un client_addr;
        socklen_t          len     = sizeof(client_addr);
        int                msg_len = (int)recvfrom(listen_fd,
                                    buf,
                                    MSG_MAX_LENGTH,
                                    0,
                                    (struct sockaddr *)&client_addr,
                                    &len);
        if (msg_len < 0)
        {
            fprintf(stderr, "recvfrom error\n");
            return -1;
        }
        if (sendto(listen_fd, buf, msg_len, 0, (struct sockaddr *)&client_addr, len) < 0)
        {
            fprintf(stderr, "sendto error\n");
            return -1;
        }
    }

    return 0;
}
