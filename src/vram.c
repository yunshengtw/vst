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
#include "vpage.h"
#include "checker.h"

typedef struct {
    vpage_t *pages;
    uint32_t size;
    uint32_t ptr;
} rw_buf_t;

typedef struct {
    vpage_t pages[VST_DRAM_SIZE / VST_BYTES_PER_PAGE];
} ram_t;

/* emulated DRAM */
uint8_t __attribute__((section (".dram"))) dram[VST_DRAM_SIZE];
static ram_t vram;
static rw_buf_t rbuf, wbuf;
static uint8_t vers[VST_MAX_LBA];

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

uint32_t vst_get_rbuf_ptr(void)
{
    return rbuf.ptr;
}

uint32_t vst_get_wbuf_ptr(void)
{
    return wbuf.ptr;
}

int open_ram(uint64_t raddr, uint32_t rsize, uint64_t waddr, uint32_t wsize)
{
    memset(dram, 0, VST_DRAM_SIZE);

    for (int i = 0; i < VST_DRAM_SIZE / VST_BYTES_PER_PAGE; i++) {
        vram.pages[i].tagged = 0;
        vram.pages[i].data =
                (uint8_t *)(uint64_t)(VST_DRAM_BASE + i * VST_BYTES_PER_PAGE);
    }
    record(LOG_RAM, "DRAM @ %x of size %u B\n", VST_DRAM_BASE, VST_DRAM_SIZE);

    assert(raddr >= VST_DRAM_BASE && raddr < VST_DRAM_BASE + VST_DRAM_SIZE);
    assert((raddr - VST_DRAM_BASE) % VST_BYTES_PER_PAGE == 0);
    rbuf.pages = &vram.pages[(raddr - VST_DRAM_BASE) / VST_BYTES_PER_PAGE];
    rbuf.size = rsize;
    rbuf.ptr = 0;
    record(LOG_RAM, "Read buffer @ %lx of size %u\n", raddr, rsize);

    assert(waddr >= VST_DRAM_BASE && waddr < VST_DRAM_BASE + VST_DRAM_SIZE);
    assert((waddr - VST_DRAM_BASE) % VST_BYTES_PER_PAGE == 0);
    wbuf.pages = &vram.pages[(waddr - VST_DRAM_BASE) / VST_BYTES_PER_PAGE];
    wbuf.size = wsize;
    wbuf.ptr = 0;
    record(LOG_RAM, "Write buffer @ %lx of size %u\n", waddr, wsize);

    record(LOG_RAM, "Virtual RAM initialized\n");
    return 0;
}

void close_ram(void)
{
}

/* TODO: version checking */
void send_to_wbuf(uint32_t lba, uint32_t n_sect)
{
    uint32_t l, r, m, s;

    l = lba;
    r = n_sect;
    s = lba % VST_SECTORS_PER_PAGE;
    while (r > 0) {
        if (s + r < VST_SECTORS_PER_PAGE)
            m = r;
        else
            m = VST_SECTORS_PER_PAGE - s;

        tag_page(&wbuf.pages[wbuf.ptr]);
        for (uint32_t i = 0; i < m; i++) {
            /* fill in lba */
            wbuf.pages[wbuf.ptr].lbas[s + i] = l + i;
            vers[l + i]++;
        }
        wbuf.ptr = (wbuf.ptr + 1) % wbuf.size;

        l += m;
        s = 0;
        r -= m;
    }
}

void recv_from_rbuf(uint32_t lba, uint32_t n_sect)
{
    uint32_t l, r, m, s;

    l = lba;
    r = n_sect;
    s = lba % VST_SECTORS_PER_PAGE;
    while (r > 0) {
        if (s + r < VST_SECTORS_PER_PAGE)
            m = r;
        else
            m = VST_SECTORS_PER_PAGE - s;

        chk_lpn_consistent(&rbuf.pages[rbuf.ptr], l, s, m, vers);
        //printf("vst: %u\n", rbuf.ptr);
        rbuf.ptr = (rbuf.ptr + 1) % rbuf.size;

        l += m;
        s = 0;
        r -= m;
    }
}

vpage_t *vram_vpage_map(uint64_t dram_addr)
{
    assert(dram_addr >= VST_DRAM_BASE &&
            dram_addr < VST_DRAM_BASE + VST_DRAM_SIZE);
    return &vram.pages[(dram_addr - VST_DRAM_BASE) / VST_BYTES_PER_PAGE];
}
