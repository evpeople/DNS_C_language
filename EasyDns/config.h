#include <uv.h>
#include "log.h"
#include "hlist.h"

uv_loop_t *loop;
uv_udp_t send_socket;
uv_udp_t recv_socket;

void initDNS(struct hashMap **hashMap);