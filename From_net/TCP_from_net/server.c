#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 1234
#define BACKLOG 1
#define MAXDATASIZE 100

int main(void)
{
    int listenfd, connectfd, n, m;
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t addrlen;
    char buf[MAXDATASIZE];

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket() error.\n");
        exit(1);
    }

    int opt = SO_REUSEADDR;

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("bind() error.\n");
        exit(1);
    }

    if (listen(listenfd, BACKLOG) == -1)
    {
        perror("listen() error.\n");
        exit(1);
    }

    addrlen = sizeof(client);

    while (1)
    {
        if ((connectfd = accept(listenfd, (struct sockaddr *)&client, &addrlen)) == -1)
        {
            perror("accept() error.\n");
            exit(1);
        }
        printf("[!] You get a connection from %s, the port is %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        send(connectfd, "Welcome to my server\n", 22, 0);

        while ((n = read(connectfd, buf, MAXDATASIZE)) > 0)
        {
            int m = strlen(buf);
            char rever[m];

            for (int i = 0; i < m; i++)
            {
                rever[m - i - 1] = buf[i];
            }
            rever[m] = '\0';
            printf("[>] Send reverse string:%s\n", buf);

            if (strcmp(rever, "tiuq") == 0)
            {
                exit(1);
            }
            write(connectfd, rever, n);
        }

        close(connectfd);
    }
    close(listenfd);
}
