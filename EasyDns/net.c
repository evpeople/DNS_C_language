#include "net.h"
#include "hlist.h"
#include "log.h"
#include <string.h>
#include <time.h>

#define DNS_MAX_PACKET 548
#define IP_LEN 40
#define ANS_LEN 1024
#define CACHELEN 299

#define NOTFOUND 1
#define CANTGIVE 2
#define GOTIT 3
#define FROMFAR 4

extern struct hashMap *hashMap;
extern struct hashMap *cacheMap;
extern uv_udp_t send_socket;
extern uv_udp_t recv_socket;

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
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    fflush(NULL);
    int stateCode = 0;
    dbg_info("get packet\n");
    char *reply = malloc(sizeof(char) * ANS_LEN);
    size_t replyLen = ANS_LEN;
    if (nread < 0)
    {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        // uv_close((uv_handle_t *)handl, NULL);
        free(buf->base);
        return;
    }
    char *rawmsg = malloc(sizeof(char) * (buf->len));
    memcpy(rawmsg, buf->base, buf->len);

    if (isQuery(rawmsg))
    {
        // dbg_ip(rawmsg, 60);
        dbg_info("in is query\n");
        char *domain = malloc(sizeof(char) * (buf->len));
        strcpy(domain, rawmsg + sizeof(struct HEADER));
        getAddress(&domain);
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
            uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
            struct sockaddr_in dnsAdd;

            dbg_ip(rawmsg, 10);
            addCacheMap(&rawmsg, addr);
            uint16_t id = *(uint16_t *)rawmsg;
            printf("##before   %x  ######################\n", id);
            if (id)
            {
                printf("##after     ######################\n");
                dbg_ip(rawmsg, 10);
                uv_ip4_addr("10.3.9.4", 53, &dnsAdd);
                uv_buf_t recvBuf = uv_buf_init(rawmsg, reply - reply + 0x7a);
                //中继DNS
                uv_udp_send(req, handl, &recvBuf, 1, (const struct sockaddr *)&dnsAdd, succse_send_cb);
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
        makeDnsHead(rawmsg, "", FROMFAR, &reply);
        uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
        uv_buf_t recvBuf;
        struct sockaddr *address = malloc(sizeof(struct sockaddr));
        dbg_info("before memcpy %hu ^^^^ %hu\n", id, cacheForId[id].key);
        memcpy(address, cacheForId[id].value, sizeof(struct sockaddr));
        dbg_info("after memcpy %hu\n", id);
        recvBuf = uv_buf_init(rawmsg, reply - reply + 0x5a);
        uv_udp_send(req, handl, &recvBuf, 1, address, succse_send_cb);
        // free(reply);
        // free(rawmsg);
        return;
        //暂且为空，可能需要解决序号问题更改相应。
    }
    //发送构造好的相应。
    if (stateCode != NOTFOUND)
    {
        dbg_info("\n\n reply is \n\n");
        uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
        uv_buf_t recvBuf;
        recvBuf = uv_buf_init(rawmsg, reply - reply + 0x5a);
        uv_udp_send(req, handl, &recvBuf, 1, addr, succse_send_cb);
    }

    free(reply);
    free(rawmsg);
    dbg_info("wan cheng udp_send\n");
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
}
