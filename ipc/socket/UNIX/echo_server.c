// 服务器
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_SOCKET_PATH "/tmp/echo_unix_socket"
#define BACKLOG          5
#define MSG_MAX_LENGTH   100

void echo(int client_fd);
void readLine(int fd, char *buf);

void signalHandler(int signo)   
{
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

    int listen_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        fprintf(stderr, "socket error\n");
        return -1;
    }

    struct sockaddr_un unix_socket_addr;
    memset(&unix_socket_addr, 0, sizeof(unix_socket_addr));
    unix_socket_addr.sun_family = AF_LOCAL;
    strcpy(unix_socket_addr.sun_path, UNIX_SOCKET_PATH);
    if (bind(listen_fd, (const struct sockaddr *)&unix_socket_addr, sizeof(unix_socket_addr)) < 0)
    {
        fprintf(stderr, "bind error\n");
        return -1;
    }

    if (listen(listen_fd, BACKLOG) < 0)
    {
        fprintf(stderr, "listen error\n");
        return -1;
    }

    for (;;)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
        {
            fprintf(stderr, "accept error\n");
            return -1;
        }

        switch (fork())
        {
            case -1:
            {
                fprintf(stderr, "fork error\n");
                return -1;
            }
            case 0:
            {
                echo(client_fd);
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return 0;
}

void echo(int client_fd)
{
    char buf[MSG_MAX_LENGTH + 1] = {0};
    for (;;)
    {
        readLine(client_fd, buf);
        int msg_len = (int)strlen(buf);
        if (write(client_fd, buf, msg_len) != msg_len)
        {
            fprintf(stderr, "write error\n");
            exit(EXIT_FAILURE);   
        }
    }
}

void readLine(int fd, char *buf)
{
    int i = 0;
    for (; i < MSG_MAX_LENGTH; i++)
    {
        switch (read(fd, buf + i, 1))
        {
            case 1:
            {
                break;
            }
            case 0:
            {
                exit(EXIT_FAILURE);   
                break;
            }
            case -1:
            {
                fprintf(stderr, "read error\n");
                exit(EXIT_FAILURE);  
                break;
            }
            default:
            {
                assert(0);
            }
        }

        if (buf[i] == '\n')
        {
            i++;
            break;
        }
    }

    buf[i] = '\0';
}
