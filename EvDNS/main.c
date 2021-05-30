#include "config.h"
#include "net.h"

int main(int argc, char **argv)
{
    loop = uv_default_loop();
    config(argc, argv);
    struct hashMap *hashMap;
    struct sockaddr_in recv_addr;
    initDNS(&hashMap, &recv_addr);
    uv_udp_recv_start(&recv_socket, alloc_buffer, dealWithPacket);
}