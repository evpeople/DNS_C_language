#include <uv.h>
#include <stdbool.h>
// #define DNS_MAX_PACK_SIZ 1024
struct HEADER
{
    unsigned id : 16;    /* query identification number */
    unsigned rd : 1;     /* recursion desired */
    unsigned tc : 1;     /* truncated message */
    unsigned aa : 1;     /* authoritive answer */
    unsigned opcode : 4; /* purpose of message */
    unsigned qr : 1;     /* response flag */
    unsigned rcode : 4;  /* response code */
    unsigned cd : 1;     /* checking disabled by resolver */
    unsigned ad : 1;     /* authentic data from named */
    unsigned z : 1;      /* unused bits, must be ZERO */
    unsigned ra : 1;     /* recursion available */
    uint16_t qdcount;    /* number of question entries */
    uint16_t ancount;    /* number of answer entries */
    uint16_t nscount;    /* number of authority entries */
    uint16_t arcount;    /* number of resource entries */
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
void strToIp(char *ans, char *ip);

void strToStr(char *ans, char *sentence);

int lenOfQuery(char *rawmsg);
void makeDnsRR(char *buf, char *ip, int state);

void makeDnsHead(char *rawmsg, char *ans, int stateCode, char **reply);
void getAddress(char **rawMsg);
void succse_send_cb(uv_udp_send_t *req, int status);
void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void dealWithPacket(uv_udp_t *handl, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);
bool isQuery(char *rawMsg);