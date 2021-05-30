#include "net.h"
#define DNS_MAX_PACKET 1024

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    // static char slab[DNS_MAX_PACK_SIZE];
    buf->base = malloc(sizeof(char) * DNS_MAX_PACKET);
    buf->len = DNS_MAX_PACKET;
}
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    char *reply;
    size_t replyLen;
    if (nread < 0)
    {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t *)req, NULL);
        free(buf->base);
        return;
    }
    char *rawmsg = buf;
    fprintf(stdout, "xxxxx%x", rawmsg);
    if (isQuery(rawmsg))
    {
        //查询表

        //构造响应
    }
    else
    {
        //暂且为空，可能需要解决序号问题更改相应。
    }

    //发送构造好的相应。
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    uv_buf_t recvBuf = uv_buf_init(reply, replyLen);
    uv_udp_send(req, handl, &recvBuf, 1, addr, succse_send_cb);
}
bool isQuery(char *rawMsg)
{
}