#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERVER_PORT 8888 //这个宏似乎在头文件中没有定义。
int main(int argc, char *argv[])
{
    int s;
    int len;
    char *servName;
    int servPort;
    char *string;
    char buffer[256 + 1];
    struct sockaddr_in servAddr;

    if (argc != 4)
    {
        printf("%d", argc);
        printf("error not enough arguments");
        exit(1);
    }
    servName = argv[1];
    servPort = atoi(argv[2]);
    string = argv[3];

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(8888);
    s = socket(PF_INET, SOCK_DGRAM, 0);
    if ((s < 0))
    {
        perror("error:socket failed");
        exit(1);
    }
    char buf[256] = "TEST UDP MSG!\n";
    sendto(s, string, sizeof(string), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));
    memset(buf, 0, 256);
    recvfrom(s, buf, 256, 0, NULL, NULL);

    buf[256] = '\0';
    printf("echo string received\n");
    printf("%s", buf);
    fputs(buffer, stdout);

    close(s);

    exit(0);
}