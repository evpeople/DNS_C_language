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

#define SERVER_PORT 8910 //这个宏似乎在头文件中没有定义。
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
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(servPort);

    if ((s = socket(PF_INET, SOCK_DGRAM, 0) == -1))
    {
        perror("error:socket failed");
        exit(1);
    }

    len = sendto(s, string, strlen(string), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));

    recvfrom(s, buffer, len, 0, NULL, NULL);

    buffer[len] = '\0';
    printf("echo string received");
    fputs(buffer, stdout);

    close(s);

    exit(0);
}