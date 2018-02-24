/**
 * logger.h
 * Authors: Yun-Sheng Chang
 */

#ifndef LOGGER_H
#define LOGGER_H

#define LOG_GENERAL 0
#define LOG_IO 1
#define LOG_FLASH 2
#define LOG_RAM 3
#define LOG_MISC 4
#define LOG_MAX 10

#define ENABLE_LOG_GENERAL 1
#define ENABLE_LOG_IO 0
#define ENABLE_LOG_FLASH 0
#define ENABLE_LOG_RAM 0
#define ENABLE_LOG_MISC 0

int open_logger(char *fname);
void close_logger(void);
void __attribute__((format(printf, 2, 3))) record(int type, const char *fmt, ...);

#endif // LOGGER_H
