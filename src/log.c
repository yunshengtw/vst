/**
 * log.c
 * Authors: Yun-Sheng Chang
 */

#include <stdio.h>
#include <stdarg.h>
#include "log.h"

static FILE *fp_log;
static int loggable[LOG_MAX];

int open_log(char *fname)
{
    if (fname != NULL) {
        fp_log = fopen(fname, "w");
        if (fp_log == NULL)
            return 1;
    }

    loggable[LOG_GENERAL] = ENABLE_LOG_GENERAL;
    loggable[LOG_IO] = ENABLE_LOG_IO;
    loggable[LOG_FLASH] = ENABLE_LOG_FLASH;
    loggable[LOG_RAM] = ENABLE_LOG_RAM;
    loggable[LOG_MISC] = ENABLE_LOG_MISC;
    for (int i = LOG_MISC + 1; i < LOG_MAX; i++)
        loggable[i] = 0;

    return 0;
}

void close_log(void)
{
    if (fp_log != NULL)
        fclose(fp_log);
}

void __attribute__((format(printf, 2, 3))) record(int type, const char *fmt, ...)
{
    if (!fp_log || !loggable[type])
        return;

    switch (type) {
    case LOG_GENERAL:
        fprintf(fp_log, "[General] ");
        break;
    case LOG_IO:
        fprintf(fp_log, "[IO] ");
        break;
    case LOG_FLASH:
        fprintf(fp_log, "[Flash] ");
        break;
    case LOG_RAM:
        fprintf(fp_log, "[RAM] ");
        break;
    case LOG_MISC:
        fprintf(fp_log, "[Misc] ");
        break;
    }

    va_list ap; 
    va_start(ap, fmt);
    vfprintf(fp_log, fmt, ap);
    va_end(ap);
}
