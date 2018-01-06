/**
 * stat.h
 * Authors: Yun-Sheng Chang
 */

#ifndef STAT_H
#define STAT_H

void inc_byte_read(uint64_t n_byte);
void inc_byte_write(uint64_t n_byte);
void inc_flash_read(uint64_t n_page);
void inc_flash_write(uint64_t n_page);
void inc_flash_cb(uint64_t n_page);
void inc_flash_erase(uint64_t n_blk);
uint64_t get_byte_write(void);
int open_stat(void);
void close_stat(void);

#endif // STAT_H
