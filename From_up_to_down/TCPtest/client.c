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

int main(int argc, char *argv[])
{
    int s;
    int n;
    char *servName;
    uint16_t servPort;
    char *string;
    int len;
    char buffer[256 + 1];
    char *ptr = buffer;
    struct hostent *he;
    struct sockaddr_in serverAddr;

    if ((he = gethostbyname(argv[1])) == NULL)
    {
        perror("gethostbyname() error.\n");
        exit(1);
    }

    if (argc != 3)
    {
        printf("ERROR:3 argu needed");
        exit(1);
    }
    // servName = argv[1]; //maybe have problem
    servPort = 8889;
    string = "TEST TCP ";
    servName = argv[1];

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    // serverAddr.sin_addr = *((struct in_addr *)he->h_addr);
    // printf("%s", *he->h_addr_list);
    fflush(stdout);
    inet_pton(AF_INET, servName, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(servPort);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR:SOCKERT creation failed");
        exit(1);
    }
    int i;
    if (i = connect(s, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("连接失败");
        exit(1);
    }
    send(s, string, strlen(string), 0);

    int maxLen = 256;
    while ((n = recv(s, ptr, maxLen, 0)) > 0)
    {
        ptr += n;
        maxLen -= n;
        len += n;
    }

    buffer[len] = '\0';
    fflush(stdout);

    printf("ECHOED STRING RECE");
    fflush(stdout);

    close(s);
    exit(0);
}