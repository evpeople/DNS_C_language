#include "log.h"
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

#include <string.h>

#define DBG_DEBUG 0x01
#define DBG_INFO 0x02
#define DBG_WARNING 0x04
#define DBG_ERROR 0x08
#define DBG_TEMP 0x10
static time_t epoch;

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