/**
 * vram.h
 * Authors: Yun-Sheng Chang
 */

#ifndef VRAM_H
#define VRAM_H

#include <stdint.h>

/* RAM APIs */
uint8_t vst_read_dram_8(uint64_t const addr);
uint16_t vst_read_dram_16(uint64_t const addr);
uint32_t vst_read_dram_32(uint64_t const addr);
void vst_write_dram_8(uint64_t const addr, uint8_t const val);
void vst_write_dram_16(uint64_t const addr, uint16_t const val);
void vst_write_dram_32(uint64_t const addr, uint32_t const val);
void vst_set_bit_dram(uint64_t const base_addr, uint32_t const bit_offset);
void vst_clr_bit_dram(uint64_t const base_addr, uint32_t const bit_offset);
uint32_t vst_tst_bit_dram(uint64_t const base_addr, uint32_t const bit_offset);
void vst_memset(uint64_t const addr, uint32_t const val, uint32_t const len);
void vst_memcpy(uint64_t const dst, uint64_t const src, uint32_t const len);
uint32_t vst_mem_search_min(uint64_t const addr, uint32_t const unit, uint32_t const size);
uint32_t vst_mem_search_max(uint64_t const addr, uint32_t const unit, uint32_t const size);
uint32_t vst_mem_search_equ(uint64_t const addr, uint32_t const unit, 
                       uint32_t const size, uint32_t const val);
int open_ram(void);
void close_ram(void);

#endif // VRAM_H
