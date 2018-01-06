/**
 * stat.c
 * Authors: Yun-Sheng Chang
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

extern int pass;
static uint64_t byte_read, byte_write;
static uint64_t cnt_flash_read, cnt_flash_write, cnt_flash_cb, cnt_flash_erase;

void inc_byte_read(uint64_t n_byte)
{
    byte_read += n_byte;
}

void inc_byte_write(uint64_t n_byte)
{
    byte_write += n_byte;
}

void inc_flash_read(uint64_t n_page)
{
    cnt_flash_read += n_page;
}

void inc_flash_write(uint64_t n_page)
{
    cnt_flash_write += n_page;
}

void inc_flash_cb(uint64_t n_page)
{
    cnt_flash_cb += n_page;
}

void inc_flash_erase(uint64_t n_blk)
{
    cnt_flash_erase += n_blk;
}

uint64_t get_byte_write(void)
{
    return byte_write;
}

int open_stat(void)
{
    byte_read = 0;
    byte_write = 0;
    cnt_flash_read = 0;
    cnt_flash_write = 0;
    cnt_flash_cb = 0;
    cnt_flash_erase = 0;
    return 0;
}

void close_stat(void)
{
    printf("----------Statistic Results----------\n");
    if (pass)
        printf("Pass!\n");
    else
        printf("Fail!\n");
    printf("Total read (MB): %" PRIu64 "\n", byte_read / (1024 * 1024));
    printf("Total write (MB): %" PRIu64 "\n", byte_write / (1024 * 1024));
    printf("Total flash read (pages): %" PRIu64 "\n", cnt_flash_read);
    printf("Total flash write (pages): %" PRIu64 "\n", cnt_flash_write);
    printf("Total flash copyback (pages): %" PRIu64 "\n", cnt_flash_cb);
    printf("Total flash erase (blocks): %" PRIu64 "\n", cnt_flash_erase);
    printf("----------Statistic Results----------\n");
}
