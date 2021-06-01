#ifndef lprintf

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stdio.h>

    extern FILE *log_file;
    extern unsigned int get_ms(void);

    int lprintf(const char *format, ...);
    int __v_lprintf(const char *format, va_list arg_ptr);

    void dbg_temp(char *fmt, ...);
    void dbg_error(char *fmt, ...);
    void dbg_warning(char *fmt, ...);
    void dbg_info(char *fmt, ...);
    void dbg_debug(char *fmt, ...);
    void dbg_ip(unsigned char *, int);
    void config(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif