#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include "log.h"
#define __int64 long long

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "log.h"

FILE *log_file = NULL;

#define bool int
#define true 1
#define false 0

static int __v_printf(const char *format, va_list arg_ptr);

#define MAX_WIDTH (10 * 1024)

static unsigned long skip_to(const char *format)
{
    unsigned long i;
    for (i = 0; format[i] && format[i] != '%'; ++i)
        ;
    return i;
}

#define tee_output(buf, len)               \
    do                                     \
    {                                      \
        fwrite(buf, 1, len, stdout);       \
        if (log_file)                      \
            fwrite(buf, 1, len, log_file); \
    } while (0)

static int output(const char *str, int len)
{
    static bool sol = true; /* start of line */
    unsigned int ms, n;
    char timestamp[32];
    const char *head, *tail, *end = str + len;

    for (head = tail = str; tail < end; head = tail)
    {
        while (tail < end && *tail++ != '\n')
            ;
        if (sol)
        {
            ms = get_ms();
            n = sprintf(timestamp, "%03d.%03d ", ms / 1000, ms % 1000);
            tee_output(timestamp, n);
        }
        tee_output(head, tail - head);
        sol = tail[-1] == '\n';
    }
    return len;
}

static int write_pad(unsigned int len, int pad_ch)
{
    const char *pad;
    int n;

    if ((int)len <= 0)
        return 0;

    if (pad_ch == '0')
        pad = "0000000000000000";
    else
        pad = "                ";

    for (n = 0; len > 15; len -= 16, n += 16)
        output(pad, 16);

    if (len > 0)
        n += output(pad, len);

    return n;
}

static int int64_str(char *s, int size, unsigned __int64 i, int base, char upcase)
{
    char *p;
    unsigned int j = 0;

    s[--size] = 0;

    p = s + size;

    if (base == 0 || base > 36)
        base = 10;

    j = 0;
    if (i == 0)
    {
        *(--p) = '0';
        j = 1;
    }

    while (p > s && i != 0)
    {
        p--;
        *p = (char)(i % base + '0');
        if (*p > '9')
            *p += (upcase ? 'A' : 'a') - '9' - 1;
        i /= base;
        j++;
    }

    memmove(s, p, j + 1);

    return j;
}

#define F_SIGN 0x0001
#define F_UPCASE 0x0002
#define F_HASH 0x0004
#define F_LEFT 0x0008
#define F_SPACE 0x0010
#define F_PLUS 0x0020
#define F_DOT 0x0040

static int output_string(
    const char *str, int size, int prefix_len,
    int width, int precision,
    int flag, char pad, char precision_pad)
{
    const char *prefix;
    int len = 0;

    if (width == 0 && precision == 0)
    {
        output(str, size);
        return size;
    }

    prefix = str;

    if (prefix_len)
    {
        str += prefix_len;
        size -= prefix_len;
        width -= prefix_len;
    }

    /* These are the cases for 1234 or "1234" respectively:
        %.6u -> "001234"
        %6u  -> "  1234"
        %06u -> "001234"
        %-6u -> "1234  "
        %.6s -> "1234"
        %6s  -> "  1234"
        %06s -> "  1234"
        %-6s -> "1234  "
        %6.5u -> " 01234"
        %6.5s -> "  1234"
        In this code, for %6.5s, 6 is width, 5 is precision.
        flag_dot means there was a '.' and precision is set.
        flag_left means there was a '-'.
        sz is 4 (strlen("1234")).
        pad will be '0' for %06u, ' ' otherwise.
        precision_pad is '0' for %u, ' ' for %s.
    */

    if ((flag & F_DOT) && width == 0)
        width = precision;

    if (!(flag & F_DOT))
        precision = size;

    /* do left-side padding with spaces */
    if (!(flag & F_LEFT) && pad == ' ')
        len += write_pad(width - precision, ' ');

    if (prefix_len)
    {
        output(prefix, prefix_len);
        len += prefix_len;
    }

    /* do left-side padding with '0' */
    if (!(flag & F_LEFT) && pad == '0')
        len += write_pad(width - precision, '0');

    /* do precision padding */
    len += write_pad(precision - size, precision_pad);

    /* write actual string */
    output(str, size);
    len += size;

    /* do right-side padding */
    if (flag & F_LEFT)
        len += write_pad(width - precision, pad);

    return len;
}

static int output_integer(
    unsigned __int64 num, int opt_long, char type,
    int width, int precision,
    int flag, int base, char *prefix, char pad)
{
    char buf[128], *s;
    int sz, prefix_len, n;
    bool is_negative;

    if (precision > width)
        width = precision;

    s = buf + 1;
    if (type == 'p')
    {
        if (num == 0)
        {
            s = "(nil)";
            return output_string(s, strlen(s), 0, width, precision, flag, ' ', ' ');
        }
    }

    strcpy(s, prefix);
    sz = strlen(s);
    prefix_len = sz;

    is_negative = false;
    if (flag & F_SIGN)
    {
        if ((signed __int64)num < 0)
        {
            num = -(signed __int64)num;
            is_negative = true;
        }
    }

    if (opt_long == 1)
        num &= (unsigned long)-1;
    else if (opt_long == 0)
        num &= (unsigned int)-1;
    else if (opt_long == -1)
        num &= 0xffff;
    else if (opt_long == -2)
        num &= 0xff;

    n = int64_str(s + sz, sizeof(buf) - sz - 1, num, base, flag & F_UPCASE);

    /* When 0 is printed with an explicit precision 0, the output is empty. */
    if ((flag & F_DOT) && n == 1 && s[sz] == '0')
    {
        if (precision == 0 || prefix_len > 0)
            sz = 0;
        prefix_len = 0;
    }
    else
        sz += n;

    if (is_negative)
    {
        prefix_len = 1;
        *(--s) = '-';
        sz++;
    }
    else if ((flag & F_SIGN) && (flag & (F_PLUS | F_SPACE)))
    {
        prefix_len = 1;
        *(--s) = (flag & F_PLUS) ? '+' : ' ';
        sz++;
    }

    if (precision > 0)
        pad = ' ';

    if (sz - prefix_len > precision)
        precision = sz - prefix_len;

    return output_string(s, sz, prefix_len, width, precision, flag, pad, '0');
}

static int output_double(double d,
                         char type, int width, int precision, int flag, int pad)
{
    char fmt[64], buf[256], *s, *p;
    int prefix_len = 0, sz;

    if (width == 0)
        width = 1;
    if (!(flag & F_DOT))
        precision = 6;
    if (d < 0.0)
        prefix_len = 1;

    s = buf + 1;

    sprintf(fmt, "%%%d.%d%c", width, precision, type);
    sprintf(s, fmt, d);

    for (p = s; *p == ' '; p++)
        ;

    for (;;)
    {
        *s = *p;
        if (*p == '\0')
            break;
        s++;
        p++;
    }
    s = buf + 1;
    sz = strlen(s);

    if ((flag & (F_PLUS | F_SPACE)) && d >= 0)
    {
        prefix_len = 1;
        *(--s) = (flag & F_PLUS) ? '+' : ' ';
        sz++;
    }

    if ((flag & F_HASH) && type == 'f' && strchr(s, '.') == NULL)
        s[sz++] = '.';

    if (width < sz)
        width = sz;
    flag &= ~F_DOT;

    return output_string(s, sz, prefix_len, width, precision, flag, pad, '0');
}

/*
   %M  (Memory block)  %M, %0M, %#0M
       0 -- LEFT PAD 0 for byte value 0~15
       # -- PREFIX length: (123) 
*/

static int output_memory_block(const unsigned char *ptr, int n,
                               int width, int precision, int flag, int pad)
{
    static const char *char_set = "0123456789abcdef";
    char str[256], *s;
    int len = 0;

    if (ptr == NULL)
        return output_string("(null)", 6, 0, width, precision, flag, pad, ' ');

    s = str;
    if (flag & F_HASH)
        s += sprintf(s, "(%d) ", n);

    if (n > (sizeof(str) - 32) / 3)
        width = precision = 0;

    while (n > 0)
    {
        if (pad != '0' && *ptr < 0x10)
            *s++ = char_set[*ptr];
        else
        {
            *s++ = char_set[*ptr / 16];
            *s++ = char_set[*ptr % 16];
        }

        n--;
        if (n > 0)
            *s++ = ' ';

        ptr++;
        if (s - str > sizeof(str) - 4)
        {
            len += output_string(str, s - str, 0, width, precision, flag, pad, ' ');
            s = str;
        }
    }

    if (s != str)
        len += output_string(str, s - str, 0, width, precision, flag, pad, ' ');

    return len;
}

int __v_lprintf(const char *format, va_list arg_ptr)
{
    unsigned int len = 0;
    int err = errno;
    char *s;
    unsigned char *ptr;
    int flag;
    signed int n;
    unsigned char ch, pad;

    signed int opt_long;

    unsigned int base;
    unsigned int width, precision;

    __int64 num;
    char *prefix;
    int prefix_len; /* 0x 0X 0 - + ' ' */

    while (*format)
    {

        n = skip_to(format);
        if (n)
        {
            output(format, n);
            len += n;
            format += n;
        }

        if (*format != '%')
            continue;

        pad = ' ';
        flag = 0;
        opt_long = 0;
        prefix_len = 0;
        prefix = "";

        width = 0;
        precision = 0;
        num = 0;

        ++format;

    next_option:
        switch (ch = *format++)
        {
        case 0:
            return -1;
            break;

        /* FLAGS */
        case '#':
            flag |= F_HASH;
            goto next_option;

        case 'h':
            --opt_long;
            goto next_option;

        case 'q':
        case 'L':
            ++opt_long;
        case 'z':
        case 'l':
            ++opt_long;
            goto next_option;

        case '-':
            flag |= F_LEFT;
            goto next_option;

        case ' ':
            flag |= F_SPACE;
            goto next_option;

        case '+':
            flag |= F_PLUS;
            goto next_option;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (flag & F_DOT)
                return -1;
            width = strtoul(format - 1, (char **)&s, 10);
            if (width > MAX_WIDTH)
                return -1;
            if (ch == '0' && !(flag & F_LEFT))
                pad = '0';
            format = s;
            goto next_option;

        case '*':
            if ((n = va_arg(arg_ptr, int)) < 0)
            {
                flag |= F_LEFT;
                n = -n;
            }
            if ((width = (unsigned long)n) > MAX_WIDTH)
                return -1;
            goto next_option;

        case '.':
            flag |= F_DOT;
            if (*format == '*')
            {
                n = va_arg(arg_ptr, int);
                ++format;
            }
            else
            {
                n = strtol(format, (char **)&s, 10);
                format = s;
            }
            precision = n < 0 ? 0 : n;
            if (precision > MAX_WIDTH)
                return -1;
            goto next_option;

        /* print a char or % */
        case 'c':
            ch = (char)va_arg(arg_ptr, int);
        case '%':
            output(&ch, 1);
            ++len;
            break;

        /* print a string */
        case 'm':
        case 's':
            s = ch == 'm' ? strerror(err) : va_arg(arg_ptr, char *);
            if (s == NULL)
                s = "(null)";
            n = strlen(s);
            if ((flag & F_DOT) && n > (signed)precision)
                n = precision;
            flag &= ~F_DOT;
            len += output_string(s, n, 0,
                                 width, 0, flag, ' ', ' ');
            break;

        /* print an integer value */
        case 'b':
            base = 2;
            goto print_num;

        case 'p':
            prefix = "0x";
            opt_long = sizeof(void *) / sizeof(long);
        case 'X':
            if (ch == 'X')
                flag |= F_UPCASE;
        case 'x':
            base = 16;
            if (flag & F_HASH)
                prefix = ch == 'X' ? "0X" : "0x";
            goto print_num;

        case 'd':
        case 'i':
            flag |= F_SIGN;
        case 'u':
            base = 10;
            goto print_num;

        case 'o':
            base = 8;
            if (flag & F_HASH)
                prefix = "0";
        print_num:
            if (opt_long > 0)
            {
                if (opt_long > 1)
                    num = va_arg(arg_ptr, __int64);
                else
                    num = (__int64)va_arg(arg_ptr, long);
            }
            else
                num = (__int64)va_arg(arg_ptr, int);

            len += output_integer(num, opt_long, ch,
                                  width, precision, flag, base, prefix, pad);
            break;

        /* print a floating point value */
        case 'g':
        case 'F':
        case 'f':
        case 'e':
        case 'E':
            len += output_double(va_arg(arg_ptr, double), ch,
                                 width, precision, flag, pad);
            break;

        /* print a memory block */
        case 'M':
            ptr = va_arg(arg_ptr, unsigned char *);
            len += output_memory_block(ptr, va_arg(arg_ptr, int),
                                       width, precision, flag, pad);
            break;

        default:
            break;
        }
    }
    return len;
}

int lprintf(const char *format, ...)
{
    int n;
    va_list arg_ptr;

    va_start(arg_ptr, format);
    n = __v_lprintf(format, arg_ptr);
    va_end(arg_ptr);

    return n;
}

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
FILE *log_file;
unsigned int get_ms(void);

int lprintf(const char *format, ...);
int __v_lprintf(const char *format, va_list arg_ptr);

void dbg_temp(char *fmt, ...);
void dbg_error(char *fmt, ...);
void dbg_warning(char *fmt, ...);
void dbg_info(char *fmt, ...);
void dbg_debug(char *fmt, ...);
void config(int argc, char **argv);

#define DBG_DEBUG 0x01
#define DBG_INFO 0x02
#define DBG_WARNING 0x04
#define DBG_ERROR 0x08
#define DBG_TEMP 0x10
static time_t epoch;
#define IPLENGTH 20
#define DOMAINLENTH 400
#define MAPLENGTH 400
struct hlistHead
{
    struct hlistNode *first;
};

struct hlistNode
{
    struct hlistNode *next, **pprev;
};

struct hashMap
{
    struct hlistHead *hlist[MAPLENGTH];
};

struct domainMap
{
    char *key;   //domin
    char *value; //ip
    struct hlistNode hash;
};

static int debug_mask = 0;

static struct option intopts[] = {

    {"debug", required_argument, NULL, 'd'},
    {0, 0, 0, 0},
};

#define OPT_SHORT "?ufind:p:b:l:t:"

void config(int argc, char **argv)
{
    char fname[1024];
    int i, opt;

    strcpy(fname, "");

    while ((opt = getopt_long(argc, argv, OPT_SHORT, intopts, NULL)) != -1)
    {
        switch (opt)
        {

        case 'd':
            debug_mask = atoi(optarg);
            break;

        default:
            printf("ERROR: Unsupported option\n");
        }
    }

    if (fname[0] == 0)
    {
        strcpy(fname, argv[0]);
        if (strcasecmp(fname + strlen(fname) - 4, ".exe") == 0)
            *(fname + strlen(fname) - 4) = 0;
        strcat(fname, ".log");
    }

    if (strcasecmp(fname, "nul") == 0)
        log_file = NULL;
    else if ((log_file = fopen(fname, "w")) == NULL)
        printf("WARNING: Failed to create log file \"%s\": %s\n", fname, strerror(errno));

    lprintf("Log file \"%s\", debug mask 0x%02x\n", fname, debug_mask);
}

void dbg_debug(char *fmt, ...)
{
    va_list arg_ptr;

    if (debug_mask & DBG_DEBUG)
    {
        va_start(arg_ptr, fmt);
        __v_lprintf(fmt, arg_ptr);
        va_end(arg_ptr);
    }
}

void dbg_info(char *fmt, ...)
{
    va_list arg_ptr;

    if (debug_mask & DBG_INFO)
    {
        va_start(arg_ptr, fmt);
        __v_lprintf(fmt, arg_ptr);
        va_end(arg_ptr);
    }
}

void dbg_warning(char *fmt, ...)
{
    va_list arg_ptr;

    if (debug_mask & DBG_WARNING)
    {
        va_start(arg_ptr, fmt);
        __v_lprintf(fmt, arg_ptr);
        va_end(arg_ptr);
    }
}
void dbg_error(char *fmt, ...)
{
    va_list arg_ptr;

    if (debug_mask & DBG_ERROR)
    {
        va_start(arg_ptr, fmt);
        __v_lprintf(fmt, arg_ptr);
        va_end(arg_ptr);
    }
}
void dbg_temp(char *fmt, ...)
{
    va_list arg_ptr;

    if (debug_mask & DBG_TEMP)
    {
        va_start(arg_ptr, fmt);
        __v_lprintf(fmt, arg_ptr);
        va_end(arg_ptr);
    }
}

unsigned int get_ms(void)
{
    struct timeval tm;
    struct timezone tz;

    gettimeofday(&tm, &tz);

    return (unsigned int)(epoch ? (tm.tv_sec - epoch) * 1000 + tm.tv_usec / 1000 : 0);
}

int hashCode(char *key)
{
    ////UNIX系统使用的哈希
    char *k = key;
    unsigned long h = 0;
    while (*k)
    {
        h = (h << 4) + *k++;
        unsigned long g = h & 0xF0000000L;
        if (g)
        {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h % MAPLENGTH;
}

void createHasMap(struct hashMap *hashMap)
{
    hashMap = malloc(sizeof(struct hashMap *));
    memset(hashMap->hlist, 0, sizeof(hashMap->hlist));
    dbg_info("2m init success \n");
}
void addHashMap(char *key, char *value, struct hashMap *hashMap) //key 是 domin， value 是ip
{
    struct domainMap *node;
    node = malloc(sizeof(struct domainMap));
    node->key = malloc(strlen(key) + 1);
    node->value = malloc(strlen(value) + 1);
    strcpy(node->key, key);
    // dbg_temp("key is %s ********** yuan key %s da xiao ww %d \n ", node->key, key, strlen(node->key));
    strcpy(node->value, value);
    node->hash.next = NULL;
    node->hash.next = NULL;
    int index = hashCode(key);

    if (hashMap->hlist[index] == NULL)
    {
        hashMap->hlist[index] = malloc(sizeof(struct hlistHead));
        hashMap->hlist[index]->first = &(node->hash);
        node->hash.pprev = &(hashMap->hlist[index])->first;
    }
    else
    {
        node->hash.pprev = hashMap->hlist[index]->first->pprev;
        node->hash.next = hashMap->hlist[index]->first;
        hashMap->hlist[index]->first->pprev = &(node->hash).next;
        *(node->hash.pprev) = &(node->hash);
    }
}

void hashMapInit(struct hashMap *hashMap)
{
    FILE *fp = NULL;
    char ip[IPLENGTH];
    dbg_info("befor open success \n");
    char domain[DOMAINLENTH];
    fp = fopen("/home/wangzhe/DNS/DNS/dnsrelay.txt", "r");
    if (fp == NULL)
    {
        dbg_error("can't open file\n");
        exit(1);
    }
    dbg_info("open success \n");
    while (!feof(fp))
    {
        fscanf(fp, "%s", ip);
        fscanf(fp, "%s", domain);
        dbg_temp("ip is %s \n", ip);
        addHashMap(domain, ip, hashMap);
    }
    fclose(fp);
}

char *findHashMap(struct hashMap *hashMap, char *key)
{
    int index = hashCode(key);
    if (hashMap->hlist[index])
    {
        struct domainMap *temp = (struct domainMap *)(hashMap->hlist[index]->first - 1); //为内存偏移的起始地址
        while (temp != NULL)
        {
            if (!strcasecmp(key, temp->key))
            {
                if (!strcmp(temp->value, "0.0.0.0"))
                {
                    dbg_info("屏蔽网站");
                    return "Not have this";
                }
                dbg_info("返回的ip是%s\n", temp->value);
                return temp->value;
            }
            temp = (struct domainMap *)(temp->hash.next - 1);
        }
    }
    else
    {
        dbg_info("没有匹配到\n");
        return NULL;
    }
}

void toDomainName(char *buf)
{
    char *p = buf;
    // while (*p != 0) //以0x00 结尾
    // {
    //     if (*p >= 63) //长度限制为63
    //     {
    //         p++;
    //     }
    //     else //DNS 中用0x 的长度间隔开 ，比如0x3 说明下一段的长度是3个字母
    //     {
    //         *p = '.';
    //         p++;
    //     }
    // }
    int temp = *p;
    while (*p != 0)
    {
        for (int i = temp; i >= 0; i--)
        {
            p++;
        }
        temp = *p;
        *p = '.';
        p++;
    }

    dbg_info("%s\n", buf);
}

void handleDnsMsg(int fd, char *temp)
{
    char buf[BUFF_LEN]; //接收缓冲区，1024字节
    socklen_t len;
    int count;
    struct sockaddr_in clent_addr; //clent_addr用于记录发送方的地址信息
    while (1)
    {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(clent_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&clent_addr, &len); //recvfrom是拥塞函数，没有数据就一直拥塞
        if (count == -1)
        {
            printf("recieve data fail!\n");
            return;
        }
        temp = malloc(400);
        strcpy(temp, buf + sizeof(struct HEADER));
        dbg_info("domain is %s \n", temp);
        toDomainName(temp);
        // printf("client:%s\n", buf); //打印client发过来的信息
        // memset(buf, 0, BUFF_LEN);
        // sprintf(buf, "I have recieved %d bytes data!\n", count); //回复client
        // printf("server:%s\n", buf);                              //打印自己发送的信息给
        // sendto(fd, buf, BUFF_LEN, 0, (struct sockaddr *)&clent_addr, len); //发送信息给client，注意使用了clent_addr结构体指针
    }
}

int initServer()
{
    int server_fd, ret;
    struct sockaddr_in ser_addr;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET:IPV4;SOCK_DGRAM:UDP
    if (server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);       //端口号，需要网络序转换

    ret = bind(server_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if (ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }
    return server_fd;
    // handleDnsMsg(server_fd); //处理接收到的数据

    // close(server_fd);
}

int main(int argc, char **argv)
{
    config(argc, argv);

    lprintf("\nde\n");
    dbg_info("befor 7 init success \n");
    struct hashMap *hashMap;
    dbg_info("befor 3 init success \n");
    // createHasMap(hashMap);
    dbg_info("befor init success \n");
    // hashMapInit(hashMap);
    FILE *fp = NULL;
    char ip[IPLENGTH];
    dbg_info("befor open success \n");
    char domain[DOMAINLENTH];
    fp = fopen("/home/wangzhe/DNS/DNS/dnsrelay.txt", "r");
    if (fp == NULL)
    {
        dbg_error("can't open file\n");
        exit(1);
    }
    dbg_info("open success!!!!!!!!! \n");
    fclose(fp);

    int fd = initServer();
    char *temp;
    handleDnsMsg(fd, temp);
    dbg_info("after handle domain is %s\n", temp);
    char *ans = findHashMap(hashMap, temp);
    dbg_info("got ip is %s\n", ans);
    return 0;
}
