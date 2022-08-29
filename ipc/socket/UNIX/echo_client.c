// 客户端
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_SOCKET_PATH "/tmp/echo_unix_socket"
#define MSG_MAX_LENGTH   100

int main(void)
{
    int socket_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        fprintf(stderr, "socker error\n");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, UNIX_SOCKET_PATH);
    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        fprintf(stderr, "sonnect error\n");
        return -1;
    }

    char buf[MSG_MAX_LENGTH + 1] = {0};
    for (;;)
    {
        fgets(buf, MSG_MAX_LENGTH, stdin);
        int len = (int)strlen(buf);
        if (write(socket_fd, buf, len) != len)
        {
            fprintf(stderr, "write error\n");
            return -1;
        }
        if (read(socket_fd, buf, len) != len)
        {
            fprintf(stderr, "read error\n");
            return -1;
        }
        printf("%s", buf);
    }

    return 0;
}
