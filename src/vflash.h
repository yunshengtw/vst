/**
 * vflash.h
 * Authors: Yun-Sheng Chang
 */

#ifndef VFLASH_H
#define VFLASH_H

#include <stdint.h>

/* flash memory APIs */
void vst_read_page(uint32_t const bank, uint32_t const blk, uint32_t const page, 
               uint32_t const sect, uint32_t const n_sect, uint64_t const dram_addr,
               uint32_t const lpn, uint8_t const is_host_data);
void vst_write_page(uint32_t const bank, uint32_t const blk, uint32_t const page,
                uint32_t const sect, uint32_t const n_sect, uint64_t const dram_addr,
                uint32_t const lpn, uint8_t const is_host_data);
void vst_copyback_page(uint32_t const bank, uint32_t const blk_src, uint32_t const page_src,
                   uint32_t const blk_dst, uint32_t const page_dst);
void vst_erase_block(uint32_t const bank, uint32_t const blk);

int open_flash(void);
void close_flash(void);

#endif // VFLASH_H
