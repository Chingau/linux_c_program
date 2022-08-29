// 客户端
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_SOCKET_PATH       "/tmp/echo_unix_socket"
#define MSG_MAX_LENGTH         100
#define SOCKET_PATH_MAX_LENGTH 50

void signalHandler(int signo) 
{
    char socket_path[SOCKET_PATH_MAX_LENGTH] = {0};
    sprintf(socket_path, "/tmp/echo_unix_socket_%ld", (long)getpid());    
    unlink(socket_path);                                                
    exit(EXIT_SUCCESS);                                            
}

int main(void)
{
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        fprintf(stderr, "signal error\n");
        return -1;
    }

    int socket_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        fprintf(stderr, "socket error\n");
        return -1;
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_LOCAL;
    strcpy(server_addr.sun_path, UNIX_SOCKET_PATH);

    struct sockaddr_un client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sun_family = AF_LOCAL;
    sprintf(client_addr.sun_path, "/tmp/echo_unix_socket_%ld", (long)getpid());

    if (bind(socket_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
    {
        fprintf(stderr, "bind error\n");
        return -1;
    }

    char buf[MSG_MAX_LENGTH + 1] = {0};
    for (;;)
    {
        fgets(buf, MSG_MAX_LENGTH, stdin);
        int msg_len = (int)strlen(buf);
        if (sendto(socket_fd,
                   buf,
                   msg_len,
                   0,
                   (struct sockaddr *)&server_addr,
                   sizeof(server_addr)) < 0)
        {
            fprintf(stderr, "sendto error\n");  
            return -1;
        }
        if (recvfrom(socket_fd, buf, MSG_MAX_LENGTH, 0, NULL, NULL) < 0)
        {
            fprintf(stderr, "recvfrom error\n");
            return -1;
        }
        printf("%s", buf);
    }

    return 0;
}
