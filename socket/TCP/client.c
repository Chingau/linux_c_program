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

    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed.");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    //serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET, SERV_IP, &serv_addr.sin_addr.s_addr);
    if ((connect(lfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("connect failed.");
        close(lfd);
        return -1;
    }

    char buf[BUF_SIZE];
    int n;
    for (;;) {
        printf("please input a string:\n");
        memset(buf, 0, BUF_SIZE);
        fgets(buf, BUF_SIZE, stdin);
        write(lfd, buf, strlen(buf));
        memset(buf, 0, BUF_SIZE);
        n = read(lfd, buf, BUF_SIZE);
        buf[n] = '\0';
        printf("the string form server: %s\r\n", buf);
    }

    close(lfd);
    return 0;
}
