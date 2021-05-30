#include <uv.h>
#include <stdbool.h>
// #define DNS_MAX_PACK_SIZ 1024
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
bool isQuery(char *rawMsg);