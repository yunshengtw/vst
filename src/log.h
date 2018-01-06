/**
 * log.h
 * Authors: Yun-Sheng Chang
 */

#ifndef LOG_H
#define LOG_H

#define LOG_GENERAL 0
#define LOG_FLASH 1
#define LOG_BUF 2
#define LOG_MISC 3
#define LOG_MAX 10

#define ENABLE_LOG_GENERAL 1
#define ENABLE_LOG_FLASH 1
#define ENABLE_LOG_BUF 1
#define ENABLE_LOG_MISC 1

int open_log(char *fname);
void close_log(void);
void __attribute__((format(printf, 2, 3))) record(int type, const char *fmt, ...);

#endif // LOG_H
