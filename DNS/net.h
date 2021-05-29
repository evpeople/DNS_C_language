#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include "log.h"

#define SERVER_PORT 8888
#define BUFF_LEN 256

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

void toDomainName(char *buf);
char *handleDnsMsg(int fd, char *temp);
void makeDnsAns(unsigned char *x, char *ip);
int initServer();
