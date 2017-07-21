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
u8 vst_read_dram_8(u64 const addr);
u16 vst_read_dram_16(u64 const addr);
u32 vst_read_dram_32(u64 const addr);
void vst_write_dram_8(u64 const addr, u8 const val);
void vst_write_dram_16(u64 const addr, u16 const val);
void vst_write_dram_32(u64 const addr, u32 const val);
void vst_set_bit_dram(u64 const base_addr, u32 const bit_offset);
void vst_clr_bit_dram(u64 const base_addr, u32 const bit_offset);
u32 vst_tst_bit_dram(u64 const base_addr, u32 const bit_offset);
void vst_memset(u64 const addr, u32 const val, u32 const len);
void vst_memcpy(u64 const dst, u64 const src, u32 const len);
u32 vst_mem_search_min(u64 const addr, u32 const unit, u32 const size);
u32 vst_mem_search_max(u64 const addr, u32 const unit, u32 const size);
u32 vst_mem_search_equ(u64 const addr, u32 const unit, 
                       u32 const size, u32 const val);

/* init */
void init_vst(void);

#endif // VST_H

