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

char *handleDnsMsg(int fd, char *temp)
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
            return NULL;
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
        //Rcode  is 0000  //todo : 应该根据源数据判断 0.0.0.0 则设计为5
        // | 1 0000 0 0 0   或用来设置1 与用来设置0 0x80
        // ^ 1 1111 0 1 1   0xFB
        // struct HEADER *forX = (struct HEADER *)x;
        memcpy(x, buf, sizeof(buf));
        struct HEADER *forX = (struct HEADER *)x;
        ((struct HEADER *)x)->aa = 0;
        ((struct HEADER *)x)->qr = 1;
        ((struct HEADER *)x)->rcode = 0;
        ((struct HEADER *)x)->ra = 0;
        // *(x + 3) = '\0';
        dbg_info("id is %x  \n", x);
        strcpy(temp, buf + sizeof(struct HEADER));
        dbg_info("domain is %s \n", temp);
        toDomainName(temp);

        // makeDnsAns(x, "0.0.0.0");

        {
            struct sockaddr_in DnsSrvAddr;
            bzero(&DnsSrvAddr, sizeof(DnsSrvAddr));
            DnsSrvAddr.sin_family = AF_INET;

            inet_aton("10.3.9.4", &DnsSrvAddr.sin_addr);
            DnsSrvAddr.sin_port = htons(53);

            unsigned int i = sizeof(DnsSrvAddr);
            len = sendto(fd, buf, 1024, 0, (struct sockaddr *)&DnsSrvAddr, sizeof(DnsSrvAddr));
            len = recvfrom(fd, buf, 1024, 0, (struct sockaddr *)&DnsSrvAddr, &i);
        }
        // return x;
        len = sizeof(clent_addr);
        // sendto(fd, buf, sizeof(struct HEADER) + sizeof(struct QUERY) + sizeof(struct ANS) + 2, 0, (struct sockaddr *)&clent_addr, len); //发送信息给client，注意使用了clent_addr结构体指针
        sendto(fd, buf, 1024, 0, (struct sockaddr *)&clent_addr, len); //发送信息给client，注意使用了clent_addr结构体指针
        // temp += 1;
        // printf("%s", temp);
        break;
        // printf("client:%s\n", buf); //打印client发过来的信息
        // memset(buf, 0, BUFF_LEN);
        // sprintf(buf, "I have recieved %d bytes data!\n", count); //回复client
        // printf("server:%s\n", buf);                              //打印自己发送的信息给
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

void makeDnsAns(unsigned char *buf, char *ip)
{
    struct HEADER *header = (struct HEADER *)buf;
    struct ANS *rr;
    if (*ip == (char)0 && *(ip + 1) == (char)0 && *(ip + 2) == (char)0 && *(ip + 3) == (char)0)
    {
        header->rcode = htons(5);
    }
    header->ancount = htons(1);
    char *dn = buf + sizeof(struct HEADER);
    //printf("%lu\n", strlen(dn));
    //dnsQuery *query = (dnsQuery *)(dn + strlen(dn) + 1);
    //query->type = htons((unsigned short)1);
    //query->classes = htons((unsigned short)1);
    char *name = dn + 2 + sizeof(struct QUERY); //给C0定位
    unsigned short *_name = (unsigned short *)name;
    *_name = htons((unsigned short)0xC00C);
    //为报文压缩

    rr = (struct ANS *)(name); //name 完紧跟着type
    rr->type = htons(1);       //0.0.0.0 的type 为 TXT？
    rr->class = htons(1);
    rr->ttl = htons(0x1565);
    // void *temp = &(rr->ttl);
    // *(&(rr->ttl) + 8) = htons(4);
    // rr->rdlength = htons(4); //如果TYPE是A CLASS 是IN 则RDATA是四个八位组的ARPA互联网地址　
    char *temp = (char *)rr + 10;
    *temp = 0;
    *(temp + 1) = 4;
    char *data = (char *)rr + 12; //到了rdata 部分
    *data = *ip;
    //ip应该需要处理。

    *(data + 1) = *(ip + 2);
    *(data + 2) = *(ip + 4);
    *(data + 3) = *(ip + 6);
}