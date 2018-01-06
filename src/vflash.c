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
#include "stat.h"

#define VST_UNKNOWN_CONTENT ((uint32_t)-1)

typedef struct {
    uint8_t *data;
    uint8_t is_erased;
    uint32_t lpn;
} flash_page_t;

typedef struct {
    flash_page_t pages[VST_PAGES_PER_BLOCK];
} flash_block_t;

typedef struct {
    flash_block_t blocks[VST_BLOCKS_PER_BANK];
} flash_bank_t;

typedef struct {
    flash_bank_t banks[VST_NUM_BANKS];
} flash_t;

/* emulated DRAM and Flash memory */
static flash_t flash;

// TODO: remove this
/* internal routines */
static void report_bug(char *msg);
#ifdef REPORT_WARNING
static void report_warning(char *msg);
#endif

/* macro functions */
#define get_page(bank, blk, page) \
        (flash.banks[(bank)].blocks[(blk)].pages[(page)])

/* public interfaces */
/* flash memory APIs */
void vst_read_page(uint32_t const bank, uint32_t const blk, uint32_t const page, 
               uint32_t const sect, uint32_t const n_sect, uint64_t const dram_addr,
               uint32_t const lpn, uint8_t const is_host_data)
{
    assert(bank < VST_NUM_BANKS);
    assert(blk < VST_BLOCKS_PER_BANK);
    assert(page < VST_PAGES_PER_BLOCK);

    flash_page_t *pp = &get_page(bank, blk, page);

    inc_flash_read(1);
    uint32_t start, length;

    start = sect * VST_BYTES_PER_SECTOR;
    length = n_sect * VST_BYTES_PER_SECTOR;

    if (start + length > VST_BYTES_PER_PAGE) {
        char msg[200];
        sprintf(msg, "read exceeds page size limit, start = %u length = %u",
            start, length);
        report_bug(msg);
        assert(0);
    }

    if (is_host_data) {
        if (!pp->is_erased && 
            pp->lpn != VST_UNKNOWN_CONTENT &&
            lpn != pp->lpn) {
            char msg[200];
            sprintf(msg, "LPN unmatched, host lpn = %u previous lpn = %u",
                lpn, pp->lpn);
            report_bug(msg);
            assert(0);
        }
        #ifdef DEBUG
        else {
            //printf("[DEBUG] LPN matched, lpn = %u\n", lpn);
        }
        #endif
    } else {
        uint8_t *addr = (uint8_t *)dram_addr;
        if (pp->data == NULL) {
            memset(addr, 0xff, length);
        } else {
            uint8_t *data = pp->data;
            memcpy(addr, &data[start], length);
        }
    }
}

void vst_write_page(uint32_t const bank, uint32_t const blk, uint32_t const page, 
                uint32_t const sect, uint32_t const n_sect, uint64_t const dram_addr,
                uint32_t const lpn, uint8_t const is_host_data)
{
    assert(bank < VST_NUM_BANKS);
    assert(blk < VST_BLOCKS_PER_BANK);
    assert(page < VST_PAGES_PER_BLOCK);

    flash_page_t *pp = &get_page(bank, blk, page);

    inc_flash_write(1);
    #ifdef DEBUG
    //printf("[DEBUG] bank = %u blk = %u page = %u\n", bank, blk, page);
    #endif
    uint32_t start, length;

    #ifdef REPORT_WARNING
    if (page != 0 && get_page(bank, blk, page - 1).is_erased) {
        char msg[200];
        sprintf(msg, "non-sequential write to blk = %u, page = %u", blk, page);
        report_warning(msg);
    }
    #endif

    start = sect * VST_BYTES_PER_SECTOR;
    length = n_sect * VST_BYTES_PER_SECTOR;

    if (start + length > VST_BYTES_PER_PAGE) {
        char msg[200];
        sprintf(msg, "write exceeds page size limit, start = %u length = %u",
            start, length);
        report_bug(msg);
        assert(0);
    }

    if (!pp->is_erased) {
        char msg[200];
        sprintf(msg, "in-place write to dirty page, "
            "bank = %u " 
            "block = %u "
            "page = %u", 
            bank, blk, page);
        report_bug(msg);
        assert(0);
    }

    pp->is_erased = 0;
    if (is_host_data) {
        pp->lpn = lpn;
    } else {
        uint8_t *data = (uint8_t *)malloc(VST_BYTES_PER_PAGE * sizeof(uint8_t));
        uint8_t *addr = (uint8_t *)dram_addr;
        memcpy(&data[start], addr, length);
        pp->data = data;
    }
}

void vst_copyback_page(uint32_t const bank, uint32_t const blk_src, uint32_t const page_src,
                   uint32_t const blk_dst, uint32_t const page_dst)
{
    inc_flash_cb(1);
    #ifdef DEBUG
    //printf("[DEBUG] src = (%u, %u) dst = (%u, %u)\n", 
    //        blk_src, page_src, blk_dst, page_dst);
    #endif

    assert(bank < VST_NUM_BANKS);
    assert(blk_src < VST_BLOCKS_PER_BANK);
    assert(page_src < VST_PAGES_PER_BLOCK);
    assert(blk_dst < VST_BLOCKS_PER_BANK);
    assert(page_dst < VST_PAGES_PER_BLOCK);

    if (!get_page(bank, blk_dst, page_dst).is_erased) {
        char msg[200];
        sprintf(msg, "in-place write to dirty page, "
            "bank = %u " 
            "block = %u "
            "page = %u", 
            bank, blk_dst, page_dst);
        report_bug(msg);
        assert(0);
    }

    get_page(bank, blk_dst, page_dst).is_erased = 0;
    get_page(bank, blk_dst, page_dst).lpn =
        get_page(bank, blk_src, page_src).lpn;

    // TODO: maybe remove this
    #ifdef REPORT_WARNING
    //if (get_page(bank, blk_src, page_src).is_erased == 1)
    //    report_warning("copyback a clean page");
    #endif

    uint8_t *data = get_page(bank, blk_src, page_src).data;
    if (data != NULL) {
        get_page(bank, blk_dst, page_dst).data = 
                (uint8_t *)malloc(VST_BYTES_PER_PAGE * sizeof(uint8_t));
        memcpy(get_page(bank, blk_dst, page_dst).data, 
               get_page(bank, blk_src, page_src).data, 
               VST_BYTES_PER_PAGE);
    }
}

void vst_erase_block(uint32_t const bank, uint32_t const blk)
{
    inc_flash_erase(1);
    uint32_t i;

    for (i = 0; i < VST_PAGES_PER_BLOCK; i++) {
        get_page(bank, blk, i).is_erased = 1;
        uint8_t *data = get_page(bank, blk, i).data;
        if (data != NULL) {
            free(data);
            get_page(bank, blk, i).data = NULL;
        }
        get_page(bank, blk, i).lpn = VST_UNKNOWN_CONTENT;
    }
}

int open_flash(void)
{
    uint32_t i, j, k;

    for (i = 0; i < VST_NUM_BANKS; i++) {
        for (j = 0; j < VST_BLOCKS_PER_BANK; j++) {
            for (k = 0; k < VST_PAGES_PER_BLOCK; k++) {
                get_page(i, j, k).data = NULL;
                get_page(i, j, k).is_erased = 1;
                get_page(i, j, k).lpn = VST_UNKNOWN_CONTENT;
            }
        }
    }
    printf("[INFO] Flash initialized\n");
    return 0;
}

void close_flash(void)
{
}

static void report_bug(char *msg)
{
    printf("[Bug] %s\n", msg);
}

#ifdef REPORT_WARNING
static void report_warning(char *msg)
{
    printf("[Warning] %s\n", msg);
}
#endif
