#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include "log.h"
#include "hlist.h"
#include "net.h"

#define DNS_MAX_PACK_SIZE 5120

uv_loop_t *loop;
uv_udp_t send_socket;
uv_udp_t recv_socket;
void sv_send_cb(uv_udp_send_t *req, int status)
{
    dbg_info("succ\n");
    // PLOG(LDEBUG, "[Server]\tSend Successful\n");
    // uv_close((uv_handle_t *)req->handle, close_cb);
    // free(req);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    // static char slab[DNS_MAX_PACK_SIZE];
    buf->base = malloc(sizeof(char) * DNS_MAX_PACK_SIZE);
    buf->len = DNS_MAX_PACK_SIZE;
}

void on_read(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t)); // 事件处理句柄，类似标识id
    uv_buf_t sndbuf;
    if (nread < 0)
    {
        fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t *)req, NULL);
        free(buf->base);
        return;
    }
    getaddrinfo char sender[17] = {0};
    uv_ip4_name((const struct sockaddr_in *)addr, sender, 16);
    fprintf(stderr, "Recv from %s\n", sender);
    {
        int len;
        struct sockaddr_in DnsSrvAddr;
        bzero(&DnsSrvAddr, sizeof(DnsSrvAddr));
        DnsSrvAddr.sin_family = AF_INET;

        inet_aton("10.3.9.4", &DnsSrvAddr.sin_addr);
        DnsSrvAddr.sin_port = htons(53);

        unsigned int i = sizeof(DnsSrvAddr);
        len = sendto(fd, buf, 1024, 0, (struct sockaddr *)&DnsSrvAddr, sizeof(DnsSrvAddr));
        len = recvfrom(fd, buf, 1024, 0, (struct sockaddr *)&DnsSrvAddr, &i);
        uv_udp_send()
    }

    // ... DHCP specific code
    uv_udp_send(req, handl, &sndbuf, 1, addr, sv_send_cb);
}

一般而言，大多数网络应用服务器端软件都是 I / O 密集型系统，服务器系统大部分的时间都花费在等待数据的输入输出上了，而不是计算，如果 CPU 把时间花在等待 I / O 操作上，就白白浪费了 CPU 的处理能力了，更重要的是，此时可能还有大量的客户端请求需要处理，而 CPU 却在等待 I / O 无法脱身。最终，以此方式工作的服务器吞吐量极低，需要更多的服务支撑业务，导致成本升高。

        为了充分利用 CPU 的计算能力，就需要避免让 CPU 等待 I /
        O 操作完成能够抽出身来做其他工作，例如，接收更多请求，等 I / O 操作完成之后再来进一步处理。这就有了 非阻塞 I / O。异步 I / O 给编程带来了一定的麻烦，因为同步思维对于人来说更自然、更容易，也不易于调试，但是实际上限时世界原本偏向异步的。如何在 I / O 操作完成后能够让 CPU 回来继续完成工作也需要更复杂的流程逻辑控制，这些都带来了一定的设计难度。不过幸好，聪明的开发者设计了 Event -
    Driven 编程模型解决了此问题。基于此模型，也衍生出高性能 IO 常见实现模式：Reactor 模式，该模式有很多变体。Redis、Nginx、Netty、java.NIO 都采用类似的模式来解决高并发的问题，libuv 自然也不例外，实现了事件驱动的异步 IO。Reactor 模式采用同步 I / O，Proactor 是一个采用异步 I / O 的模式。

        int
        main()
{
    loop = uv_default_loop();

    uv_udp_init(loop, &recv_socket);
    struct sockaddr_in recv_addr;
    uv_ip4_addr("0.0.0.0", 6800, &recv_addr);
    uv_udp_bind(&recv_socket, (const struct sockaddr *)&recv_addr, UV_UDP_REUSEADDR);
    uv_udp_recv_start(&recv_socket, alloc_buffer, on_read);
}