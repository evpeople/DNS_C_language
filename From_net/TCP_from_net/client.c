/*client.c*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define PORT 1234
#define MAXDATASIZE 100

int main(int argc, char *argv[])
{
    int fd, numbytes, i;
    char buf[MAXDATASIZE];
    char sendStr[MAXDATASIZE], recStr[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;
    if (argc != 2)
    {
        printf("Usage: %s  <IP address>\n", argv[0]);
        exit(1);
    }
    if ((he = gethostbyname(argv[1])) == NULL)
    {
        perror("gethostbyname() error.\n");
        exit(1);
    }

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error.\n");
        exit(1);
    }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);

    if (i = connect(fd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("connect() error\n.");
        exit(1);
    }

    if ((numbytes = recv(fd, buf, MAXDATASIZE, 0)) == -1)
    {
        perror("recv() error.\n");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("[>] %s", buf);

    while (i != -1)
    {
        printf("[*] Please input string: ");
        scanf("%s", &sendStr);
        sendStr[strlen(sendStr)] = '\0';
        if (strcmp(sendStr, "quit") == 0)
        {
            write(fd, sendStr, strlen(sendStr) + 1);
            exit(1);
        }
        write(fd, sendStr, strlen(sendStr) + 1);
        read(fd, recStr, MAXDATASIZE);
        printf("[>] Received reverse string:%s\n", recStr);
    }
    close(fd);
}
