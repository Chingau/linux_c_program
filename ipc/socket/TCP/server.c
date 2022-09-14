#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <string.h>

#define SERV_PORT 6666
#define SERV_IP "127.0.0.1"
#define BUF_SIZE 512

int main(int argc, const char *argv[])
{
    int lfd;
    struct sockaddr_in serv_addr;
    int i, n;

    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed.");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("bind failed.");
        close(lfd);
        return -1;
    }

    if ((listen(lfd, 128)) < 0) {
        perror("listen failed.");
        close(lfd);
        return -1;
    }

    int cfd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    if ((cfd = accept(lfd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        perror("accept failed.");
        close(lfd);
        return -1;
    }

    char buf[BUF_SIZE];
    
    for (;;) {
        memset(buf, 0, BUF_SIZE);
        if ((n = read(cfd, buf, sizeof(buf))) < 0) {
            perror("read failed.");
            close(lfd);
            close(cfd);
            return -1;
        }

        printf("read form client: %s\r\n", buf);

        for (i = 0; i < n; ++i)
            buf[i] = toupper(buf[i]);

        write(cfd, buf, n);
    }

    close(cfd);
    close(lfd);

    return 0;
}
