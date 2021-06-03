
#include "config.h"

extern uv_loop_t *loop;
extern uv_udp_t send_socket;
extern uv_udp_t recv_socket;
extern struct hashMap *cacheMap;

void initDNS(struct hashMap **hashMap)
{
    createHasMap(hashMap);
    createHasMap(&cacheMap);
    hashMapInit(hashMap);
    uv_udp_init(loop, &recv_socket);
    struct sockaddr_in recv_addr;
    uv_ip4_addr("0.0.0.0", 6801, &recv_addr);
    uv_udp_bind(&recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
    dbg_info("ip udp bind success\n");
}