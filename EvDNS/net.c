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
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    uv_buf_t recvBuf;
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
        /* code */
    }
}
bool isQuery(char *rawMsg)
{
}