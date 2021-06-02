#include "config.h"
#include "net.h"

uv_loop_t *loop;
struct hashMap *hashMap;
struct hashMap *cacheMap;
uv_udp_t send_socket;
uv_udp_t recv_socket;
//h245 的解码
//多发的包
//数据更新
int main(int argc, char **argv)
{
    loop = uv_default_loop();
    config(argc, argv);
    // struct sockaddr_in *recv_addr;
    initDNS(&hashMap);
    uv_udp_recv_start(&recv_socket, alloc_buffer, dealWithPacket);
    uv_run(loop, UV_RUN_DEFAULT);
}