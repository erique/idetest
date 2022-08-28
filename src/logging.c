#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <exec/types.h>
#include <inline/exec.h>

#include "logging.h"
#include "version.h"

#define RawPutChar(___ch) \
    LP1NR(516, RawPutChar , BYTE, ___ch, d0,\
          , EXEC_BASE_NAME)

static void raw_put_char(uint32_t c __asm("d0"))
{
    struct ExecBase* SysBase = *(struct ExecBase**)4;
    RawPutChar(c);
}

int kprintf(const char* format, ...)
{
    if (format == NULL)
        return 0;

    va_list arg;
    va_start(arg, format);
    int ret = kvprintf(format, arg);
    va_end(arg);
    return ret;
}

int kvprintf(const char* format, va_list ap)
{
    __asm volatile("bchg.b #1,0xbfe001": : : "cc", "memory");

    struct ExecBase* SysBase = *(struct ExecBase**)4;
    RawDoFmt((STRPTR)format, ap, (__fpt)raw_put_char, NULL);
    return 0;
}

#define SGR_RESET   0
#define SGR_BLACK   30
#define SGR_RED     31
#define SGR_GREEN   32
#define SGR_YELLOW  33
#define SGR_BLUE    34
#define SGR_MAGENTA 35
#define SGR_CYAN    36
#define SGR_WHITE   37

#define STR_RESET   "\x1b[" STR(SGR_RESET)   "m"
#define STR_BLACK   "\x1b[" STR(SGR_BLACK)   "m"
#define STR_RED     "\x1b[" STR(SGR_RED)     "m"
#define STR_GREEN   "\x1b[" STR(SGR_GREEN)   "m"
#define STR_YELLOW  "\x1b[" STR(SGR_YELLOW)  "m"
#define STR_BLUE    "\x1b[" STR(SGR_BLUE)    "m"
#define STR_MAGENTA "\x1b[" STR(SGR_MAGENTA) "m"
#define STR_CYAN    "\x1b[" STR(SGR_CYAN)    "m"
#define STR_WHITE   "\x1b[" STR(SGR_WHITE)   "m"

int klog(int prio, const char* tag, const char* fmt, ... )
{
    uint8_t color = SGR_RESET;
    uint8_t c;
    char buf[8];

    switch (prio)
    {
        case LOG_FATAL:
            c = 'F';
            color = SGR_MAGENTA;
            break;

        case LOG_ERROR:
            c = 'E';
            color = SGR_RED;
            break;

        case LOG_WARN:
            c = 'W';
            color = SGR_YELLOW;
            break;

        case LOG_INFO:
            c = 'I';
            color = SGR_GREEN;
            break;

        case LOG_DEBUG:
            c = 'D';
            color = SGR_CYAN;
            break;

        case LOG_VERBOSE:
            c = 'V';
            color = SGR_WHITE;
            break;

        default:
            c = '?';
            color = SGR_WHITE;
            break;
    }

    int16_t i = 0;

    for (; i < sizeof(buf) && tag[i]; ++i)
        buf[i] = tag[i];

    for (; i < sizeof(buf); ++i)
        buf[i] = ' ';

    buf[sizeof(buf) - 1] = 0;

    kprintf("\x1b[%ldm[%lc/%s] ", color, (uint32_t)c, buf);

    va_list arg;
    va_start(arg, fmt);
    int ret = kvprintf(fmt, arg);
    va_end(arg);

    kprintf("\x1b[%ldm", SGR_RESET);

    return ret;
}

static char* strcpy(char* to, const char* from)
{
    char* save = to;

    for (; (*to = *from) != '\0'; ++from, ++to);

    return (save);
}

static char* strcat(char* s, const char* append)
{
    char* save = s;

    for (; *s; ++s);

    while ((*s++ = *append++) != '\0');

    return (save);
}

static int isgraph(int c)
{
    return 0x21 <= c && c <= 0x7e;
}

void klogmem(const uint8_t* buffer, uint32_t size)
{
    uint32_t i, j, len;
    char format[150];
    char alphas[27];

    KVERBOSE("MEM", "Memory at address $%lx, size %ld bytes (%08lx - %08lx)\n", buffer, size, buffer, buffer + size - 1);

    if (LOG_LEVEL < LOG_DEBUG)
        return;

    strcpy(format, "[" STR_CYAN "$%08lx" STR_RESET "] [" STR_YELLOW "%03lx" STR_RESET "]: " STR_WHITE "%04lx %04lx %04lx %04lx %04lx %04lx %04lx %04lx ");

    for (i = 0; i < size; i += 16)
    {
        len = size - i;

        // last line is less than 16 bytes? rewrite the format string
        if (len < 16)
        {
            strcpy(format, "[" STR_CYAN "$%08lx" STR_RESET "] [" STR_YELLOW "%03lx" STR_RESET "]: " STR_WHITE);

            for (j = 0; j < 16; j += 2)
            {
                if (j < len)
                {
                    strcat(format, "%04lx");

                }
                else
                {
                    strcat(format, "____");
                }

                strcat(format, " ");
            }

        }
        else
        {
            len = 16;
        }

        // create the ascii representation
        alphas[0] = '\'';

        for (j = 0; j < len; ++j)
        {
            alphas[j + 1] = (isgraph(buffer[i + j]) ? buffer[i + j] : '.');
        }

        for (; j < 16; ++j)
        {
            alphas[j + 1] = '_';
        }

        strcpy(&alphas[j + 1], "'\n");

        uint16_t* p = (uint16_t*)&buffer[i];
        (void)p;
        kprintf(format, buffer + i, i, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
        kprintf(STR_GREEN "%s" STR_RESET, alphas);
    }
}
