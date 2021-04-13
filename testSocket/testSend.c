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
int main()
{
    int s;
    int len;
    char buffer[256];
    struct sockaddr_in servAddr;
    struct sockaddr_in cintAddr;
    int cIntAddrLen;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERVER_PORT);       // host to network short 将主机字节序格式的短整型数值转换为网络字节序格式
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long 插入ip

    if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("ERROR:SOCKET failed!");
        exit(1);
    }
    if (bind(s, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        perror("ERROR:bind failed!");
        exit(1);
    }
    for (;;)
    {
        len = recvfrom(s, buffer, sizeof(buffer), 0, (struct sockaddr *)&cintAddr, &cIntAddrLen); //没有对报文做任何加工
        sendto(s, buffer, len, 0, (struct sockaddr *)&cintAddr, sizeof(cintAddr));
    }
}