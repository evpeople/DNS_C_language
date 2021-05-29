#include "net.h"
#include "hlist.h"
void toDomainName(char *buf)
{
    char *p = buf;
    // while (*p != 0) //以0x00 结尾
    // {
    //     if (*p >= 63) //长度限制为63
    //     {
    //         p++;
    //     }
    //     else //DNS 中用0x 的长度间隔开 ，比如0x3 说明下一段的长度是3个字母
    //     {
    //         *p = '.';
    //         p++;
    //     }
    // }
    int temp = *p;
    // // *p = 0;
    // int fir = 1;
    while (*p != 0)
    {
        // if (fir)
        // {
        //     *p = 0;
        //     fir = 0;
        // }

        for (int i = temp; i >= 0; i--)
        {
            p++;
        }
        temp = *p;
        *p = '.';
        p++;
    }
    p--;
    *p = 0;
    dbg_info("%s\n", buf);
}

void handleDnsMsg(int fd, char *temp)
{
    char buf[BUFF_LEN]; //接收缓冲区，1024字节
    socklen_t len;
    int count;
    struct sockaddr_in clent_addr; //clent_addr用于记录发送方的地址信息
    while (1)
    {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(clent_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&clent_addr, &len); //recvfrom是拥塞函数，没有数据就一直拥塞
        if (count == -1)
        {
            printf("recieve data fail!\n");
            return;
        }
        // temp = malloc(400);
        unsigned char *x = malloc(sizeof(buf));
        //QR  set to 1    1
        //opcode not set  4
        //AA is 0         1
        //TC is 0         1
        //RD is same
        //RA is 0         1
        //Z is 000        3
        //Rcode  is 0000  //todo : 应该根据源数据判断
        // | 1 0000 0 0 0   或用来设置1 与用来设置0 0x80
        // ^ 1 1111 0 1 1   0xFB

        memcpy(x, buf, 4);
        *(x + 2) |= 0x81;
        *(x + 3) &= 0x00;

        *(x + 3) = '\0';
        dbg_info("id is %x  \n", x);
        strcpy(temp, buf + sizeof(struct HEADER));
        dbg_info("domain is %s \n", temp);
        toDomainName(temp);
        // temp += 1;
        // printf("%s", temp);
        break;
        // printf("client:%s\n", buf); //打印client发过来的信息
        // memset(buf, 0, BUFF_LEN);
        // sprintf(buf, "I have recieved %d bytes data!\n", count); //回复client
        // printf("server:%s\n", buf);                              //打印自己发送的信息给
        // sendto(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&clent_addr, len); //发送信息给client，注意使用了clent_addr结构体指针
    }
}

int initServer()
{
    int server_fd, ret;
    struct sockaddr_in ser_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if (server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);       //端口号，需要网络序转换

    ret = bind(server_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if (ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }
    return server_fd;
    // handleDnsMsg(server_fd); //处理接收到的数据

    // close(server_fd);
}
