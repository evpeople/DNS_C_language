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

#define SERVER_PORT 8889
int main()
{
    int ls;
    int s;
    char buffer[256];
    char *ptr = buffer;
    int len = 0;
    int maxLen = sizeof(buffer);
    int n = 0;
    int waitSize = 16;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int cIntAddrLen;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVER_PORT);

    if ((ls = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Listen socket failed!");
        exit(1);
    }
    if (bind(ls, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("binding failed!");
        exit(1);
    }

    if (listen(ls, waitSize))
    {
        perror("Error: listening failed");
        exit(1);
    }

    for (;;)
    {
        if (s = accept(ls, (struct sockaddr *)&clientAddr, &cIntAddrLen) < 0)
        {
            perror("Error accepting failed");
            exit(1);
        }

        while ((n = recv(s, ptr, maxLen, 0)) > 0)
        {
            ptr += n;
            maxLen -= n;
            len += n;
        }
        send(s, buffer, len, 0);
        close(s);
    }
}