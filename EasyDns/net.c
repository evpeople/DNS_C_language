#include "net.h"
#include "hlist.h"
#include "log.h"
#include <string.h>
#include <time.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define DNS_MAX_PACKET 548
#define IP_LEN 40
#define ANS_LEN 1024
#define CACHELEN 299

#define NOTFOUND 1
#define CANTGIVE 2
#define GOTIT 3
#define FROMFAR 4
#define SERVER_PORT 8888
#define MAXEPOLLSIZE 50

extern struct hashMap *hashMap;
extern struct hashMap *cacheMap;
uv_udp_t send_socket;
uv_udp_t recv_socket;

static struct cache cacheForId[300];

void strToIp(char *ans, char *ip)
{
    int i = 0;
    int num = 0;
    while (*ip)
    {
        if (*ip != '.')
        {
            num = 10 * num + (*ip - '0');
        }
        else
        {
            *ans = num;
            ans++;
            num = 0;
        }
        ip++;
    }
    *ans = num;
}
void strToStr(char *ans, char *sentence)
{
    while (*sentence)
    {
        *ans = *sentence;
        ans++;
        sentence++;
    }
}
int lenOfQuery(char *rawmsg)
{
    int temp;
    temp = strlen(rawmsg) + 5;
    return temp;
}
void makeDnsRR(char *buf, char *ip, int state)
{
    struct HEADER *header = (struct HEADER *)buf;
    struct ANS *rr;

    header->ancount = htons(1); //确定有多少条消息

    char *dn = buf + sizeof(struct HEADER);
    int lenght = lenOfQuery(dn);
    char *name = dn + lenght; //给C0定位
    unsigned short *_name = (unsigned short *)name;
    *_name = htons((unsigned short)0xC00C);
    //为报文压缩

    rr = (struct ANS *)(name); //设置rr的类型
    rr->class = htons(1);
    rr->ttl = htons(0x1565);

    //数据区域
    char *temp = (char *)rr + 10;
    *temp = 0;

    if (state == CANTGIVE) //屏蔽网站
    {
        header->rcode = 5;
        rr->type = htons(16); //0.0.0.0 的type 为 TXT？
        *(temp + 1) = 1;
        *(temp + 2) = strlen("it is a bad net website");
        char *data = (char *)rr + 13; //到了rdata 部分
        strToStr(data, "it is a bad net website");
    }
    else //正常应答
    {
        rr->type = htons(1); //0.0.0.0 的type 为 TXT？
        *(temp + 1) = 4;
        char *data = (char *)rr + 12; //到了rdata 部分
        *data = *ip;
        dbg_warning("ip is %s\n", ip);
        strToIp(data, ip);
        dbg_warning("ip is %s\n", data);
    }
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    static char slab[DNS_MAX_PACKET]; //使用静态的，每次给不同的地址
    buf->base = slab;
    buf->len = sizeof(slab);
    dbg_info("has alloc\n");
}

void makeDnsHead(char *rawmsg, char *ans, int stateCode, char **reply)
{
    switch (stateCode)
    {
    case NOTFOUND:

        break;
    case CANTGIVE:
        ((struct HEADER *)rawmsg)->aa = 0;
        ((struct HEADER *)rawmsg)->qr = 1;
        ((struct HEADER *)rawmsg)->rcode = htons(5);
        ((struct HEADER *)rawmsg)->ra = 0;
        break;
    case GOTIT:
        dbg_info("############################################################################3before change header\n");
        ((struct HEADER *)rawmsg)->aa = 0;
        ((struct HEADER *)rawmsg)->qr = 1;
        ((struct HEADER *)rawmsg)->rcode = 0;
        ((struct HEADER *)rawmsg)->ra = 0;
        break;
    case FROMFAR:;
        dbg_info("预备转发包\n");
        uint16_t id = *(uint16_t *)rawmsg;
        dbg_info("%x before change is %x\n", id, ((struct HEADER *)rawmsg)->id);
        ((struct HEADER *)rawmsg)->id = (cacheForId[id % CACHELEN].key);
        // *reply = (char *)&cacheForId[*(uint16_t *)rawmsg].value;
        dbg_info("%x after change is %x\n", id, ((struct HEADER *)rawmsg)->id);
        break;
    default:
        dbg_error("接收包的函数遇到严重的错误\n");
        break;
    }
}

void getAddress(char **rawMsg)
{
    char *p = *rawMsg;
    int temp = *p;
    while (*p != 0)
    {

        for (int i = temp; i >= 0; i--)
        {
            p++;
        }
        temp = *p;
        if (temp == 0)
        {
            break;
        }
        *p = '.';
    }
    (*rawMsg)++;
    dbg_info("get Address is %s\n", *rawMsg);
}
void succse_send_cb(uv_udp_send_t *req, int status)
{
    if (status == 0)
    {
        dbg_info("success send UDP\n");
    }
}
void dealWithPacket(char *buf, const struct sockaddr *addr, int fd)
{
    fflush(NULL);
    int stateCode = 0;
    dbg_info("get packet\n");
    char *reply = malloc(sizeof(char) * ANS_LEN);
    size_t replyLen = ANS_LEN;

    char *rawmsg = malloc(sizeof(char) * ANS_LEN);
    memcpy(rawmsg, buf, ANS_LEN);
    char *domain = malloc(sizeof(char) * ANS_LEN);
    strcpy(domain, rawmsg + sizeof(struct HEADER));
    getAddress(&domain);
    if (isQuery(rawmsg))
    {
        // dbg_ip(rawmsg, 60);
        dbg_info("in is query\n");

        char *ans = malloc(sizeof(char) * IP_LEN);
        findHashMap(&hashMap, domain, &ans);
        free(domain - 1);
        dbg_info("ans is %s\n", ans);
        //查询表
        dbg_info("*ans is %c\n", *ans);

        if (*ans == 'Z') //没有找到
        {
            dbg_info("not find \n");
            stateCode = NOTFOUND;
            // uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
            struct sockaddr_in dnsAdd;

            dbg_ip(rawmsg, 10);
            addCacheMap(&rawmsg, addr);
            uint16_t id = *(uint16_t *)rawmsg;
            printf("##before   %x  ######################\n", id);
            if (id)
            {
                printf("##after     ######################\n");
                dbg_ip(rawmsg, 10);
                dnsAdd.sin_family = AF_INET;
                dnsAdd.sin_addr.s_addr = inet_addr("10.3.9.4"); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
                dnsAdd.sin_port = htons(53);                    //端口号，需要网络序转换

                // uv_ip4_addr("10.3.9.4", 53, &dnsAdd);
                //     uv_buf_t recvBuf = uv_buf_init(rawmsg, reply - reply + 0x7a);
                //     //中继DNS
                //     uv_udp_send(req, handl, &recvBuf, 1, (const struct sockaddr *)&dnsAdd, succse_send_cb);
                int len = sizeof(dnsAdd);

                int x = sendto(fd, rawmsg, ANS_LEN, 0, (const struct sockaddr *)&dnsAdd, len);
                printf("%d,%s\n", x, strerror(errno));
                id = 0;
            }

            // free(reply);
            // free(rawmsg);
            //没有找到，ans已经被释放
            // free(ans);
            // return;
        }
        else if (!strcmp(ans, "0.0.0.0"))
        {
            stateCode = CANTGIVE;
        }
        else
        {
            stateCode = GOTIT;
        }
        dbg_info("state code is %d", stateCode);
        makeDnsHead(rawmsg, ans, stateCode, &reply);
        makeDnsRR(rawmsg, ans, stateCode);
        free(ans);
    }
    else
    {
        stateCode = NOTFOUND;
        uint16_t id = *(uint16_t *)rawmsg;
        dbg_info("get id is %hu\n", id);
        // char *domain = malloc(sizeof(char) * ANS_LEN);
        // strcpy(domain, rawmsg + sizeof(struct HEADER));
        // getAddress(&domain);

        // printf("%d sssssssssssssssssssssssss\n", (*(uint16_t *)(rawmsg + sizeof(struct HEADER) + lenOfQuery(rawmsg + sizeof(struct HEADER)) + 4)));
        // dbg_ip(((rawmsg + sizeof(struct HEADER) + lenOfQuery(rawmsg + sizeof(struct HEADER)) + 3)), 10);
        if ((*(uint16_t *)(rawmsg + sizeof(struct HEADER) + lenOfQuery(rawmsg + sizeof(struct HEADER)) + 3)) == 1)
        {
            // dbg_info("jingru");
            char *ip = malloc(sizeof(char) * IP_LEN);
            getIP(rawmsg, &ip);
            addHashMap(domain, ip, &cacheMap, DYNAMIC);

            free(ip);
            // dbg_ip(ip, 5);
        }

        free(domain - 1);
        makeDnsHead(rawmsg, "", FROMFAR, &reply);
        dbg_info("relly send is \n");
        dbg_ip(rawmsg, 10);
        // uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
        // uv_buf_t recvBuf;
        struct sockaddr *address = malloc(sizeof(struct sockaddr));
        // dbg_info("before memcpy %hu ^^^^ %hu\n", id, cacheForId[id].key);
        memcpy(address, cacheForId[id].value, sizeof(struct sockaddr));
        // dbg_info("after memcpy %hu\n", id);
        // recvBuf = uv_buf_init(rawmsg, reply - reply + 0x5a);
        // sendto(fd,rawmsg,0)
        int len = sizeof(*address);
        sendto(fd, rawmsg, ANS_LEN, 0, address, len);
        // uv_udp_send(req, handl, &recvBuf, 1, address, succse_send_cb);
        // free(reply);
        // free(rawmsg);
        return;
        //暂且为空，可能需要解决序号问题更改相应。
    }
    //发送构造好的相应。
    if (stateCode != NOTFOUND)
    {
        dbg_info("\n\n reply is \n\n");
        int len = sizeof(*addr);

        int asd = sendto(fd, rawmsg, ANS_LEN, 0, addr, len);
        dbg_info("%s\n", strerror(errno));

        // printf("%d\n", asd);
        // uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
        // uv_buf_t recvBuf;
        // recvBuf = uv_buf_init(rawmsg, reply - reply + 0x5a);
        // uv_udp_send(req, handl, &recvBuf, 1, addr, succse_send_cb);
    }

    free(reply);
    free(rawmsg);
    dbg_info("wan cheng udp_send\n");
}
void getIP(char *rawmsg, char **ans)
{
    // memcpy(*ans, rawmsg + sizeof(struct HEADER) + lenOfQuery(rawmsg + sizeof(struct HEADER)) + 19, 5);
    char *temp = rawmsg + (sizeof(struct HEADER) + lenOfQuery(rawmsg + sizeof(struct HEADER)) + 6);
    dbg_info("Fun getIP \n");

    dbg_ip(*temp, 20);
    inet_addr()
        // char *p = *ans;a
        // for (size_t i = 0; i < 4; i++)
        // {
        //     *p = *rawmsg;
        //     p++;
        //     rawmsg++;
        // }
        //   dbg_info("get Address is %s\n", *rawMsg);
        dbg_ip(*ans, 5);
}
void getTTl(char *raw, char **ans)
{
}
bool isQuery(char *rawMsg)
{
    return !*(rawMsg + 3); //qr==0 is Query;
}
void initCache()
{
    for (size_t i = 0; i < CACHELEN; i++)
    {
        cacheForId[i].value == NULL;
    }
}
void addCacheMap(char **rawmsg, const struct sockaddr *addr)
{
    char *p = *rawmsg;
    uint16_t id = *(uint16_t *)(*rawmsg);
    dbg_info("%x relayeasd change is %x\n", id, ((struct HEADER *)(*rawmsg))->id);
    clock_t idInCache = clock();
    cacheForId[idInCache % CACHELEN].key = id; //安装发过去的id存的
    //    uint16_t id = *(uint16_t *)rawmsg;
    if (!addr)
    {
        *(uint16_t *)(*rawmsg) = 0;
        return;
    }
    if (cacheForId[idInCache % CACHELEN].value != NULL)
    {
        free(cacheForId[idInCache % CACHELEN].value);
        cacheForId[idInCache % CACHELEN].value = NULL;
    }

    cacheForId[idInCache % CACHELEN].value = malloc(sizeof(struct sockaddr) + 4);
    memcpy(cacheForId[idInCache % CACHELEN].value, addr, sizeof(struct sockaddr));

    // ((struct HEADER *)(*rawmsg))->id = htons(idInCache % CACHELEN);
    // uint16_t id = *(uint16_t *)rawmsg;
    *(uint16_t *)(*rawmsg) = idInCache % CACHELEN;
    uint16_t id2 = *(uint16_t *)(*rawmsg);
    dbg_info("changed eded id is %hu xxxx is in %hu yyyyyy is %hu\n", id2, idInCache % CACHELEN, cacheForId[idInCache % CACHELEN].key);
    // *p = 2;
    // printf("##inside   %x  ######################\n", ((struct HEADER *)(rawmsg))->id);
    printf("inside\n");
    dbg_ip(*rawmsg, 10);
    printf("out side \n");
}
void runDns()

{

    int listenfd;
    listenfd = initSocket();

    if (listenfd == -1)
    {
        perror("can't create socket file");
        return;
    }
    if (setnonblocking(listenfd) < 0)
    {
        perror("setnonblock error");
    }

    int len;
    char buf[ANS_LEN]; //接收缓冲区，1024字节
    struct sockaddr_in clent_addr;
    len = sizeof(struct sockaddr_in);
    // int count = recvfrom(listenfd, buf, 1024, 0, (struct sockaddr *)&clent_addr, &len); //recvfrom是拥塞函数，没有数据就一直拥塞    recvfrom()
    // char *rawmsg = malloc(sizeof(char) * ANS_LEN);
    // memcpy(rawmsg, buf, ANS_LEN);
    // int asd = sendto(listenfd, buf, ANS_LEN, 0, (const struct sockaddr *)&clent_addr, len);
    // dealWithPacket(rawmsg, (struct sockaddr *)&clent_addr, listenfd);

    int epollfd = epoll_create(MAXEPOLLSIZE);
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
    // int opt = 1;
    // setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // int netTimeOut = 6000;
    // setsockopt(socket，SOL_S0CKET, SO_RCVTIMEO，(char *) & netTimeout, sizeof(int));
    // char buf[ANS_LEN]; //接收缓冲区，1024字节
    // struct sockaddr_in clent_addr;
    //get buffer
    while (1)
    {
        int nfds = epoll_wait(epollfd, events, 20, 500);
        for (size_t i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                int count = recvfrom(listenfd, buf, 1024, 0, (struct sockaddr *)&clent_addr, &len); //recvfrom是拥塞函数，没有数据就一直拥塞    recvfrom()
                char *rawmsg = malloc(sizeof(char) * ANS_LEN);
                memcpy(rawmsg, buf, ANS_LEN);
                setblocking(listenfd);
                dealWithPacket(rawmsg, (struct sockaddr *)&clent_addr, listenfd);
                setnonblocking(listenfd);
            }
            else
            {
                dbg_info("yao fa bao !!!!!!!!!!");
            }
        }
    }
}
int setnonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}

int setblocking(int sockfd)
{
    // fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) & ~O_NONBLOCK) == -1)
    {
        return -1;
    }
    return 0;
}

int initSocket()
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
}