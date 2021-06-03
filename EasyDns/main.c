#include "config.h"
#include "net.h"
#include <sys/epoll.h>
struct hashMap *hashMap;
struct hashMap *cacheMap;
//h245 的解码
//多发的包
//数据更新
int main(int argc, char **argv)
{
    config(argc, argv);
    // struct sockaddr_in *recv_addr;
    initDNS(&hashMap);

    runDns();
    // uv_udp_recv_start(&recv_socket, alloc_buffer, dealWithPacket);
    // uv_run(loop, UV_RUN_DEFAULT);
}