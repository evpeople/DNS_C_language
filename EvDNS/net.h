#include <uv.h>
#include <stdbool.h>
// #define DNS_MAX_PACK_SIZ 1024
struct HEADER
{
    uint16_t id; /* query identification number */
    uint8_t qr;
    uint8_t opcode;
    uint8_t aa;
    uint8_t tc;
    uint8_t rd;
    uint8_t ra;
    uint16_t rcode;
    uint16_t qdcount; /* number of question entries */
    uint16_t ancount; /* number of answer entries */
    uint16_t nscount; /* number of authority entries */
    uint16_t arcount; /* number of resource entries */
};
struct QUERY
{
    unsigned qname : 16;  /* recursion desired */
    unsigned qtype : 16;  /* truncated message */
    unsigned qclass : 16; /* authoritive answer */
};

struct ANS
{
    uint16_t name;
    uint16_t type;
    // unsigned short type;

    // unsigned short class;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    // void *rdata;
};

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
bool isQuery(char *rawMsg);