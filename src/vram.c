/**
 * vram.c
 * Authors: Yun-Sheng Chang
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "config.h"
#include "logger.h"
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
uint8_t vst_read_dram_8(uint64_t addr)
{
    return *(uint8_t *)addr;
}

uint16_t vst_read_dram_16(uint64_t addr)
{
    assert(!(addr & 1));

    return *(uint16_t *)addr;
}

uint32_t vst_read_dram_32(uint64_t addr)
{
    assert(!(addr & 3));

    return *(uint32_t *)addr;
}

void vst_write_dram_8(uint64_t addr, uint8_t val)
{
    *(uint8_t *)addr = val;
}

void vst_write_dram_16(uint64_t addr, uint16_t val)
{
    assert(!(addr & 1));

    *(uint16_t *)addr = val;
}

void vst_write_dram_32(uint64_t addr, uint32_t val)
{
    assert(!(addr & 3));

    *(uint32_t *)addr = val;
}

void vst_set_bit_dram(uint64_t base_addr, uint32_t bit_offset)
{
    uint64_t addr = base_addr + bit_offset / 8;
    uint32_t offset = bit_offset % 8;

    *(uint8_t *)addr = *(uint8_t *)addr | (1 << offset);
}

void vst_clr_bit_dram(uint64_t base_addr, uint32_t bit_offset)
{
    uint64_t addr = base_addr + bit_offset / 8;
    uint32_t offset = bit_offset % 8;

    *(uint8_t *)addr = *(uint8_t *)addr & ~(1 << offset);
}

uint32_t vst_tst_bit_dram(uint64_t base_addr, uint32_t bit_offset)
{
    uint64_t addr = base_addr + bit_offset / 8;
    uint32_t offset = bit_offset % 8;

    return (*(uint8_t *)addr) & (1 << offset);
}

void vst_memcpy(uint64_t dst, uint64_t src, uint32_t len)
{
    record(LOG_RAM, "memcpy: mem[0x%lx] -> mem[0x%lx] of len %u\n", src, dst, len);

    vpage_t *pp_dst, *pp_src;
    pp_dst = vram_vpage_map(dst);
    pp_src = vram_vpage_map(src);
    if (pp_src == NULL) {
        /* sram -> sram/dram */
        if (pp_dst != NULL) {
            /* sram -> dram */
            vpage_t *pp_dst_end;
            pp_dst_end = vram_vpage_map(dst + len);
            for (vpage_t *pp = pp_dst; pp <= pp_dst_end; pp++)
                untag_page(pp);
        }
        memcpy((void *)dst, (void *)src, len);
    } else {
        /* dram -> sram/dram */
        if (pp_dst == NULL) {
            /* dram -> sram */
            if (!pp_src->tagged)
                memcpy((void *)dst, (void *)src, len);
            else
                /* there's nothing we can do if dram is tagged */
                record(LOG_RAM, "Try to move tagged DRAM data to SRAM\n");
        } else {
            /* dram -> dram */
            vpage_t *pp_dst_end;
            pp_dst_end = vram_vpage_map(dst + len);
            if (!pp_src->tagged) {
                for (vpage_t *pp = pp_dst; pp <= pp_dst_end; pp++)
                    untag_page(pp);
                memcpy((void *)dst, (void *)src, len);
            } else {
                for (vpage_t *pp = pp_dst; pp <= pp_dst_end; pp++)
                    tag_page(pp);
                record(LOG_RAM, "Tagged data movement\n");
                /* only support sector-aligned tagged data copy */
                if (dst % VST_BYTES_PER_SECTOR != 0 ||
                        src % VST_BYTES_PER_SECTOR != 0 ||
                        len % VST_BYTES_PER_SECTOR != 0) {
                    abort();
                }
                int x, y, n_sect;
                x = dst % VST_BYTES_PER_PAGE / VST_BYTES_PER_SECTOR;
                y = src % VST_BYTES_PER_PAGE / VST_BYTES_PER_SECTOR;
                n_sect = len / VST_BYTES_PER_SECTOR;
                for (int i = 0; i < n_sect; i++) {
                    record(LOG_RAM, "\tmem[%p] + sec[%d] -> mem[%p] + sec[%d], lba = %u\n",
                            pp_src->data, y, pp_dst->data, x, pp_src->lbas[y]);
                    pp_dst->lbas[x] = pp_src->lbas[y];
                    x++;
                    y++;
                    if (x == VST_SECTORS_PER_PAGE) {
                        x = 0;
                        pp_dst++;
                    }
                    if (y == VST_SECTORS_PER_PAGE) {
                        y = 0;
                        pp_src++;
                    }
                }
            }
        }
    }
}

void vst_memset(uint64_t addr, uint32_t val, uint32_t len)
{
    record(LOG_RAM, "memset: mem[0x%lx] of len %u\n", addr, len);

    vpage_t *pp_tgt;
    pp_tgt = vram_vpage_map(addr);
    if (pp_tgt != NULL) {
        vpage_t *pp_tgt_end;
        pp_tgt_end = vram_vpage_map(addr + len);
        assert(pp_tgt_end);
        for (vpage_t *pp = pp_tgt; pp <= pp_tgt_end; pp++)
            untag_page(pp);
    }
    memset((void *)addr, val, len);
}

uint32_t vst_mem_search_min(uint64_t addr, uint32_t unit, uint32_t size)
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

uint32_t vst_mem_search_max(uint64_t addr, uint32_t unit, uint32_t size)
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

uint32_t vst_mem_search_equ(uint64_t addr, uint32_t unit,
                       uint32_t size, uint32_t val)
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
    if (dram_addr >= VST_DRAM_BASE &&
        dram_addr < VST_DRAM_BASE + VST_DRAM_SIZE) {
        return &vram.pages[(dram_addr - VST_DRAM_BASE) / VST_BYTES_PER_PAGE];
    }
    return NULL;
}
