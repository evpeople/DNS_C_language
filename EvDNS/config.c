
#include "config.h"

void initDNS(struct hashMap **hashMap, struct sockaddr_in *recv_addr)
{
    createHasMap(hashMap);
    hashMapInit(hashMap);
    uv_ip4_addr("0.0.0.0", 6800, &recv_addr);
    uv_udp_bind(&recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
}