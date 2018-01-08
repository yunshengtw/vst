/**
 * vflash.c
 * Authors: Yun-Sheng Chang
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "config.h"
#include "vflash.h"
#include "vram.h"
#include "vpage.h"
#include "log.h"
#include "checker.h"
#include "stat.h"

#define VST_UNKNOWN_CONTENT ((uint32_t)-1)

/* emulated DRAM and Flash memory */
static flash_t flash;

/* macro functions */
#define get_page(bank, blk, page) \
        (flash.banks[(bank)].blocks[(blk)].pages[(page)])

/* public interfaces */
/* flash memory APIs */
void vst_read_page(uint32_t const bank, uint32_t const blk, uint32_t const page,
               uint32_t const sect, uint32_t const n_sect, uint64_t const dram_addr,
               uint32_t const lpn, uint8_t const is_host_data)
{
    record(LOG_FLASH, "R: (%u, %u, %u)\n", bank, blk, page);
    inc_flash_read(1);

    assert(bank < VST_NUM_BANKS);
    assert(blk < VST_BLOCKS_PER_BANK);
    assert(page < VST_PAGES_PER_BLOCK);
/*
    uint32_t start, length;
    start = sect * VST_BYTES_PER_SECTOR;
    length = n_sect * VST_BYTES_PER_SECTOR;
    assert(start + length <= VST_BYTES_PER_PAGE);
*/
    flash_page_t *pp = &get_page(bank, blk, page);

    vpage_copy(vram_vpage_map(dram_addr), &pp->vpage, sect, n_sect);
/*
    if (is_host_data) {
        chk_lpn_consistent(pp, lpn);
    } else {
        uint8_t *addr = (uint8_t *)(dram_addr + start);
        if (pp->data == NULL) {
            memset(addr, 0xff, length);
        } else {
            uint8_t *data = pp->data;
            memcpy(addr, &data[start], length);
        }
    }
*/
}

void vst_write_page(uint32_t const bank, uint32_t const blk, uint32_t const page,
                uint32_t const sect, uint32_t const n_sect, uint64_t const dram_addr,
                uint32_t const lpn, uint8_t const is_host_data)
{
    record(LOG_FLASH, "W: (%u, %u, %u)\n", bank, blk, page);
    inc_flash_write(1);

    assert(bank < VST_NUM_BANKS);
    assert(blk < VST_BLOCKS_PER_BANK);
    assert(page < VST_PAGES_PER_BLOCK);
/*
    uint32_t start, length;
    start = sect * VST_BYTES_PER_SECTOR;
    length = n_sect * VST_BYTES_PER_SECTOR;
    assert(start + length <= VST_BYTES_PER_PAGE);
*/
    chk_non_seq_write(&flash, bank, blk, page);

    chk_overwrite(&flash, bank, blk, page);

    flash_page_t *pp = &get_page(bank, blk, page);
    pp->is_erased = 0;
    vpage_copy(&pp->vpage, vram_vpage_map(dram_addr), sect, n_sect);
/*
    if (is_host_data) {
        pp->lpn = lpn;
    } else {
        uint8_t *data = (uint8_t *)malloc(VST_BYTES_PER_PAGE * sizeof(uint8_t));
        uint8_t *addr = (uint8_t *)dram_addr;
        memcpy(&data[start], addr, length);
        pp->data = data;
    }
*/
}

void vst_copyback_page(uint32_t const bank, uint32_t const blk_src, uint32_t const page_src,
                   uint32_t const blk_dst, uint32_t const page_dst)
{
    record(LOG_FLASH, "CB: (%u, %u, %u, %u, %u)\n", bank, blk_src, page_src,
            blk_dst, page_dst);
    inc_flash_cb(1);

    assert(bank < VST_NUM_BANKS);
    assert(blk_src < VST_BLOCKS_PER_BANK);
    assert(page_src < VST_PAGES_PER_BLOCK);
    assert(blk_dst < VST_BLOCKS_PER_BANK);
    assert(page_dst < VST_PAGES_PER_BLOCK);

    chk_overwrite(&flash, bank, blk_dst, page_dst);

    flash_page_t *pp_dst, *pp_src;
    pp_dst = &get_page(bank, blk_dst, page_dst);
    pp_src = &get_page(bank, blk_src, page_src);
    pp_dst->is_erased = 0;
    vpage_copy(&pp_dst->vpage, &pp_src->vpage, 0, VST_SECTORS_PER_PAGE);
/*
    get_page(bank, blk_dst, page_dst).lpn =
        get_page(bank, blk_src, page_src).lpn;
*/
    // TODO: maybe remove this
    #ifdef REPORT_WARNING
    //if (get_page(bank, blk_src, page_src).is_erased == 1)
    //    report_warning("copyback a clean page");
    #endif
/*
    uint8_t *data = get_page(bank, blk_src, page_src).data;
    if (data != NULL) {
        get_page(bank, blk_dst, page_dst).data = 
                (uint8_t *)malloc(VST_BYTES_PER_PAGE * sizeof(uint8_t));
        memcpy(get_page(bank, blk_dst, page_dst).data, 
               get_page(bank, blk_src, page_src).data, 
               VST_BYTES_PER_PAGE);
    }
*/
}

void vst_erase_block(uint32_t const bank, uint32_t const blk)
{
    record(LOG_FLASH, "E: (%u, %u)\n", bank, blk);
    inc_flash_erase(1);

    for (uint32_t i = 0; i < VST_PAGES_PER_BLOCK; i++) {
        flash_page_t *pp;
        pp = &get_page(bank, blk, i);
        pp->is_erased = 1;
        vpage_free(&pp->vpage);
/*
        uint8_t *data = get_page(bank, blk, i).data;
        if (data != NULL) {
            free(data);
            get_page(bank, blk, i).data = NULL;
        }
        get_page(bank, blk, i).lpn = VST_UNKNOWN_CONTENT;
*/
    }
}

int open_flash(void)
{
    uint32_t i, j, k;

    for (i = 0; i < VST_NUM_BANKS; i++) {
        for (j = 0; j < VST_BLOCKS_PER_BANK; j++) {
            for (k = 0; k < VST_PAGES_PER_BLOCK; k++) {
                //get_page(i, j, k).data = NULL;
                flash_page_t *pp = &get_page(i, j, k);
                pp->is_erased = 1;
                vpage_init(&pp->vpage, NULL);
                //get_page(i, j, k).lpn = VST_UNKNOWN_CONTENT;
            }
        }
    }
    record(LOG_FLASH, "Virtual flash initialized\n");
    return 0;
}

void close_flash(void)
{
}
