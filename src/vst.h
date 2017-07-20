/**
 * vst.h
 * Authors: Yun-Sheng Chang
 */

#ifndef VST_H
#define VST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "config.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct flash_page {
    u8 *data;
    u8 is_erased;
    u32 lpn;
} flash_page_t;

typedef struct flash_block {
    flash_page_t pages[VST_PAGES_PER_BLK];
} flash_block_t;

typedef struct flash_bank {
    flash_block_t blocks[VST_BLKS_PER_BANK];
} flash_bank_t;

typedef struct flash_top {
    flash_bank_t banks[VST_NUM_BANKS];
} flash_t;

/* flash memory APIs */
void vst_read_page(u32 const bank, u32 const blk, u32 const page, 
               u32 const sect, u32 const n_sect, u64 const dram_addr,
               u32 const lpn, u8 const is_host_data);
void vst_write_page(u32 const bank, u32 const blk, u32 const page,
                u32 const sect, u32 const n_sect, u64 const dram_addr,
                u32 const lpn, u8 const is_host_data);
void vst_copyback_page(u32 const bank, u32 const blk_src, u32 const page_src,
                   u32 const blk_dst, u32 const page_dst);
void vst_erase_block(u32 const bank, u32 const blk);

/* RAM APIs */
u8 vst_read_ext_8(u64 const addr);
u16 vst_read_ext_16(u64 const addr);
u32 vst_read_ext_32(u64 const addr);
void vst_write_ext_8(u64 const addr, u8 const val);
void vst_write_ext_16(u64 const addr, u16 const val);
void vst_write_ext_32(u64 const addr, u32 const val);
void vst_set_bit_dram(u64 const base_addr, u32 const bit_offset);
void vst_clr_bit_dram(u64 const base_addr, u32 const bit_offset);
u32 vst_tst_bit_dram(u64 const base_addr, u32 const bit_offset);
void vst_mem_set_sram(u64 const addr, u32 const val, u32 const len);
void vst_mem_set_dram(u64 const addr, u32 const val, u32 const len);
void vst_mem_copy(u64 const dst, u64 const src, u32 const len);

/* init */
void init_vst(void);

#endif // VST_H
