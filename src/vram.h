/**
 * vram.h
 * Authors: Yun-Sheng Chang
 */

#ifndef VRAM_H
#define VRAM_H

#include <stdint.h>
#include "config.h"
#include "vpage.h"

/* RAM APIs */
uint8_t vst_read_dram_8(uint64_t addr);
uint16_t vst_read_dram_16(uint64_t addr);
uint32_t vst_read_dram_32(uint64_t addr);
void vst_write_dram_8(uint64_t addr, uint8_t val);
void vst_write_dram_16(uint64_t addr, uint16_t val);
void vst_write_dram_32(uint64_t addr, uint32_t val);
void vst_set_bit_dram(uint64_t base_addr, uint32_t bit_offset);
void vst_clr_bit_dram(uint64_t base_addr, uint32_t bit_offset);
uint32_t vst_tst_bit_dram(uint64_t base_addr, uint32_t bit_offset);
void vst_memset(uint64_t addr, uint32_t val, uint32_t len);
void vst_memcpy(uint64_t dst, uint64_t src, uint32_t len);
uint32_t vst_mem_search_min(uint64_t addr, uint32_t unit, uint32_t size);
uint32_t vst_mem_search_max(uint64_t addr, uint32_t unit, uint32_t size);
uint32_t vst_mem_search_equ(uint64_t addr, uint32_t unit, 
                       uint32_t size, uint32_t val);
uint32_t vst_get_rbuf_ptr(void);
uint32_t vst_get_wbuf_ptr(void);

int open_ram(uint64_t raddr, uint32_t rsize, uint64_t waddr, uint32_t wsize);
void close_ram(void);
void send_to_wbuf(uint32_t lba, uint32_t n_sect);
void recv_from_rbuf(uint32_t lba, uint32_t n_sect);
vpage_t *vram_vpage_map(uint64_t dram_addr);

#endif // VRAM_H
