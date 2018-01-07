/**
 * vram.c
 * Authors: Yun-Sheng Chang
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "config.h"
#include "log.h"
#include "vram.h"

/* emulated DRAM */
uint8_t __attribute__((section (".dram"))) dram[VST_DRAM_SIZE];

/* RAM APIs */
inline uint8_t vst_read_dram_8(uint64_t const addr)
{
    return *(uint8_t *)addr;
}

inline uint16_t vst_read_dram_16(uint64_t const addr)
{
    assert(!(addr & 1));

    return *(uint16_t *)addr;
}

inline uint32_t vst_read_dram_32(uint64_t const addr)
{
    assert(!(addr & 3));

    return *(uint32_t *)addr;
}

inline void vst_write_dram_8(uint64_t const addr, uint8_t const val)
{
    *(uint8_t *)addr = val;
}

inline void vst_write_dram_16(uint64_t const addr, uint16_t const val)
{
    assert(!(addr & 1));

    *(uint16_t *)addr = val;
}

inline void vst_write_dram_32(uint64_t const addr, uint32_t const val)
{
    assert(!(addr & 3));

    *(uint32_t *)addr = val;
}

void vst_set_bit_dram(uint64_t const base_addr, uint32_t const bit_offset)
{
    uint64_t addr = base_addr + bit_offset / 8;
    uint32_t offset = bit_offset % 8;

    *(uint8_t *)addr = *(uint8_t *)addr | (1 << offset);
}

void vst_clr_bit_dram(uint64_t const base_addr, uint32_t const bit_offset)
{
    uint64_t addr = base_addr + bit_offset / 8;
    uint32_t offset = bit_offset % 8;

    *(uint8_t *)addr = *(uint8_t *)addr & ~(1 << offset);
}

uint32_t vst_tst_bit_dram(uint64_t const base_addr, uint32_t const bit_offset)
{
    uint64_t addr = base_addr + bit_offset / 8;
    uint32_t offset = bit_offset % 8;

    return (*(uint8_t *)addr) & (1 << offset);
}

void vst_memcpy(uint64_t const dst, uint64_t const src, uint32_t const len)
{
    memcpy((void *)dst, (void *)src, len);
}

void vst_memset(uint64_t const addr, uint32_t const val, uint32_t const len)
{
    memset((void *)addr, val, len);
}

uint32_t vst_mem_search_min(uint64_t const addr, uint32_t const unit, uint32_t const size)
{
    assert(unit == 1 || unit == 2 || unit == 4);
    assert(!(addr % unit));
    assert(size != 0);

    uint32_t i;
    uint32_t idx = 0;
    if (unit == 1) {
        uint8_t *vals = (uint8_t *)addr;
        uint8_t min = vals[0];
        if (min == 0)
            return idx;
        for (i = 1; i < size; i++) {
            if (vals[i] < min) {
                min = vals[i];
                idx = i;
                if (min == 0)
                    return idx;
            }
        }
    } else if (unit == 2) {
        uint16_t *vals = (uint16_t *)addr;
        uint16_t min = vals[0];
        if (min == 0)
            return idx;
        for (i = 1; i < size; i++) {
            if (vals[i] < min) {
                min = vals[i];
                idx = i;
                if (min == 0)
                    return idx;
            }
        }
    } else {
        uint32_t *vals = (uint32_t *)addr;
        uint32_t min = vals[0];
        if (min == 0)
            return idx;
        for (i = 1; i < size; i++) {
            if (vals[i] < min) {
                min = vals[i];
                idx = i;
                if (min == 0)
                    return idx;
            }
        }
    }
    return idx;
}

uint32_t vst_mem_search_max(uint64_t const addr, uint32_t const unit, uint32_t const size)
{
    assert(!(addr % size));
    assert(unit == 1 || unit == 2 || unit == 4);
    assert(size != 0);

    uint32_t i;
    uint32_t idx = 0;
    if (unit == 1) {
        uint8_t *vals = (uint8_t *)addr;
        uint8_t max = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] > max) {
                max = vals[i];
                idx = i;
            }
        }
    } else if (unit == 2) {
        uint16_t *vals = (uint16_t *)addr;
        uint16_t max = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] > max) {
                max = vals[i];
                idx = i;
            }
        }
    } else {
        uint32_t *vals = (uint32_t *)addr;
        uint32_t max = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] > max) {
                max = vals[i];
                idx = i;
            }
        }
    }
    return idx;
}

uint32_t vst_mem_search_equ(uint64_t const addr, uint32_t const unit,
                       uint32_t const size, uint32_t const val)
{
    //TODO
    return 0;
}

int open_ram(void)
{
    memset(dram, 0, VST_DRAM_SIZE);
    record(LOG_RAM, "Virtual RAM initialized\n");
    return 0;
}

void close_ram(void)
{
}
