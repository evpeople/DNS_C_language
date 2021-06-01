#include "net.h"
#include "hlist.h"
#include "log.h"
#include <string.h>

#define DNS_MAX_PACKET 1024
#define IP_LEN 40
#define ANS_LEN 1024

#define NOTFOUND 1
#define CANTGIVE 2
#define GOTIT 3

extern struct hashMap *hashMap;
extern uv_udp_t send_socket;
extern uv_udp_t recv_socket;

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
void makeDnsRR(char *buf, char *ip)
{
    struct HEADER *header = (struct HEADER *)buf;
    struct ANS *rr;
    if (*ip == (char)0 && *(ip + 1) == (char)0 && *(ip + 2) == (char)0 && *(ip + 3) == (char)0)
    {
        header->rcode = htons(5);
    }
    header->ancount = htons(1);
    char *dn = buf + sizeof(struct HEADER);
    // //printf("%lu\n", strlen(dn));
    // //dnsQuery *query = (dnsQuery *)(dn + strlen(dn) + 1);
    // //query->type = htons((unsigned short)1);
    // //query->classes = htons((unsigned short)1);
    char *name = dn + 3 + sizeof(struct QUERY); //给C0定位
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
    dbg_warning("ip is %s\n", ip);
    strToIp(data, ip);
    dbg_warning("ip is %s\n", data);
    // *(data + 1) = *(ip + 2) - '0';
    // *(data + 2) = *(ip + 4) - '0';
    // *(data + 3) = *(ip + 6) - '0';
    // *(data + 1) = 2;
    // *(data + 2) = 4;
    // *(data + 3) = 2;
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
    // memcpy((*reply), rawmsg, DNS_MAX_PACKET);
    char *x = rawmsg;
    switch (stateCode)
    {
    case NOTFOUND:

        break;
    case CANTGIVE:
        break;
    case GOTIT:
        dbg_ip(x, 22);
        dbg_info("############################################################################3before change header\n");
        ((struct HEADER *)rawmsg)->aa = 0;
        ((struct HEADER *)rawmsg)->qr = 1;
        ((struct HEADER *)rawmsg)->rcode = 0;
        ((struct HEADER *)rawmsg)->ra = 0;
        dbg_ip(x, 22);
        makeDnsRR(rawmsg, ans);
        dbg_info("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
        dbg_ip(x, 60);
        break;
    default:
        dbg_error("接收包的函数遇到严重的错误\n");
        break;
    }
}

void getAddress(char **rawMsg)
{
    // rawMsg += sizeof(struct HEADER);
    char *p = *rawMsg;
    // for (size_t i = 0; i < 20; i++)
    // {
    //     rawMsg += 1;
    // }

    int temp = *p;

    // // *p = 0;
    // int fir = 1;
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
        // rawMsg++;
    }
    (*rawMsg)++;
    // rawMsg--;
    // *rawMsg = 0;
    dbg_info("get Address is %s\n", *rawMsg);
}
void succse_send_cb(uv_udp_send_t *req, int status)
{
    if (status == 0)
    {
        dbg_info("fa song cheng gong\n");
    }
    // dbg_info("fasongs\n");
    // PLOG(LDEBUG, "[Server]\tSend Successful\n");
    // uv_close((uv_handle_t *)req->handle, close_cb);
    // free(req);
}
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
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

    fprintf(stdout, "xxxxx%s", rawmsg);
    if (isQuery(rawmsg))
    {
        char *domain = malloc(sizeof(char) * (buf->len));

        // memcpy(domain, rawmsg, buf->len);
        strcpy(domain, rawmsg + sizeof(struct HEADER));
        getAddress(&domain);
        char *ans = malloc(sizeof(char) * IP_LEN);
        findHashMap(&hashMap, domain, &ans);
        dbg_debug("%s\n", ans);
        int stateCode = 0;
        //查询表
        if (!strcmp(ans, "Z")) //没有找到
        {
            stateCode = NOTFOUND;
        }
        else if (!strcmp(ans, "0.0.0.0"))
        {
            stateCode = CANTGIVE;
        }
        else
        {
            stateCode = GOTIT;
        }
        makeDnsHead(rawmsg, ans, stateCode, &reply);
        // free(domain);
        dbg_info("after Make DNSHEAD\n");
        dbg_ip(reply, 60);
        // free(ans);
        dbg_info("after free ans \n");
        dbg_ip(reply, 60);
    }
    else
    {
        //暂且为空，可能需要解决序号问题更改相应。
    }

    // free(rawmsg);
    dbg_info("after free rawmsg\n ");
    dbg_ip(reply, 60);
    //发送构造好的相应。
    dbg_info("kai shi fa song udp\n");
    // uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    // // fflush(NULL);
    // uv_buf_t recvBuf;
    // recvBuf = uv_buf_init(rawmsg, 2);
    dbg_info("\n\n reply is \n\n");
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    // fflush(NULL);
    uv_buf_t recvBuf;
    recvBuf = uv_buf_init(rawmsg, reply - reply + 50);
    uv_udp_send(req, handl, &recvBuf, 1, addr, succse_send_cb);
    dbg_ip(reply, 60);
    dbg_info("\n SSSSSSS\n");
    dbg_ip(recvBuf.base, 50);
    // uv_buf_t recvBuf = uv_buf_init("sc", 2);
    free(reply);
    free(rawmsg);
    // free(ans);

    // uv_udp_send(req, handl, &recvBuf, 1, addr, succse_send_cb);
    dbg_info("wan cheng udp_send\n");
}
bool isQuery(char *rawMsg)
{
    // uint16_t t = 1;
    // char *x = rawMsg;
    // char s = ((struct HEADER *)rawMsg); //qr==0 is Query;
    // struct HEADER *y = (struct HEADER *)rawMsg;
    return !*(rawMsg + 3); //qr==0 is Query;
}
