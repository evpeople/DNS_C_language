#include "net.h"
#include "hlist.h"
#include "log.h"
#include <string.h>

#define DNS_MAX_PACKET 1024
#define IP_LEN 40

#define NOTFOUND 1
#define CANTGIVE 2
#define GOTIT 3

extern struct hashMap *hashMap;
extern uv_udp_t send_socket;
extern uv_udp_t recv_socket;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    // static char slab[DNS_MAX_PACK_SIZE];
    buf->base = malloc(sizeof(char) * DNS_MAX_PACKET);
    buf->len = DNS_MAX_PACKET;
    dbg_info("over alloc\n");
}
void makeDnsPacket(char *rawmsg, char *ans, int stateCode, char **reply)
{
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
        dbg_info("succ\n");
    }

    // PLOG(LDEBUG, "[Server]\tSend Successful\n");
    // uv_close((uv_handle_t *)req->handle, close_cb);
    free(req);
}
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    dbg_info("get packet\n");
    char *reply;
    size_t replyLen;
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
        makeDnsPacket(rawmsg, ans, stateCode, &reply);

        free(domain);
        free(ans);
    }
    else
    {
        //暂且为空，可能需要解决序号问题更改相应。
    }

    free(rawmsg);
    //发送构造好的相应。
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    uv_buf_t recvBuf = uv_buf_init(reply, replyLen);
    uv_udp_send(req, handl, &recvBuf, 1, addr, succse_send_cb);
}
bool isQuery(char *rawMsg)
{
    return !(((struct HEADER *)rawMsg)->qr); //qr==0 is Query;
}
