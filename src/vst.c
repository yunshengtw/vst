/**
 * vst.c
 * Authors: Yun-Sheng Chang
 */

#include "vst.h"

typedef struct flash_page {
    u8 *data;
    u8 is_erased;
    u32 lpn;
} flash_page_t;

typedef struct flash_block {
    flash_page_t pages[VST_PAGES_PER_BLOCK];
} flash_block_t;

typedef struct flash_bank {
    flash_block_t blocks[VST_BLOCKS_PER_BANK];
} flash_bank_t;

typedef struct flash_top {
    flash_bank_t banks[VST_NUM_BANKS];
} flash_t;

// TODO: assertions

/* emulated DRAM and Flash memory */
u8 __attribute__((section (".dram"))) dram[VST_DRAM_SIZE];
static flash_t flash;

/* statistic results */
uint64_t byte_write, byte_read;
uint64_t cnt_flash_read, cnt_flash_write, cnt_flash_cb, cnt_flash_erase;

/* internal routines */
static void init_flash(void);
static void init_dram(void);
static void init_statistic(void);
static void report_bug(char *msg);

/* macro functions */
#define get_page(bank, blk, page) \
        (flash.banks[(bank)].blocks[(blk)].pages[(page)])

/* public interfaces */
/* flash memory APIs */
void vst_read_page(u32 const bank, u32 const blk, u32 const page, 
               u32 const sect, u32 const n_sect, u64 const dram_addr,
               u32 const lpn, u8 const is_host_data)
{
    cnt_flash_read++;
    u32 start, length;

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
        if (!get_page(bank, blk, page).is_erased && 
            lpn != get_page(bank, blk, page).lpn) {
            char msg[200];
            sprintf(msg, "LPN unmatched, host lpn = %u previous lpn = %u",
                lpn, get_page(bank, blk, page).lpn);
            report_bug(msg);
            assert(0);
        }
        #ifdef DEBUG
        else {
            //printf("[DEBUG] LPN matched, lpn = %u\n", lpn);
        }
        #endif
    } else {
        u8 *addr = (u8 *)dram_addr;
        if (get_page(bank, blk, page).data == NULL) {
            memset(addr, 0xff, length);
        } else {
            u8 *data = get_page(bank, blk, page).data;
            memcpy(addr, &data[start], length);
        }
    }
}

void vst_write_page(u32 const bank, u32 const blk, u32 const page, 
                u32 const sect, u32 const n_sect, u64 const dram_addr,
                u32 const lpn, u8 const is_host_data)
{
    cnt_flash_write++;
    #ifdef DEBUG
    //printf("[DEBUG] bank = %u blk = %u page = %u\n", bank, blk, page);
    #endif
    u32 start, length;

    start = sect * VST_BYTES_PER_SECTOR;
    length = n_sect * VST_BYTES_PER_SECTOR;

    if (start + length > VST_BYTES_PER_PAGE) {
        char msg[200];
        sprintf(msg, "write exceeds page size limit, start = %u length = %u",
            start, length);
        report_bug(msg);
        assert(0);
    }

    if (!get_page(bank, blk, page).is_erased) {
        char msg[200];
        sprintf(msg, "in-place write to dirty page, "
            "bank = %u " 
            "block = %u "
            "page = %u", 
            bank, blk, page);
        report_bug(msg);
        assert(0);
    }

    get_page(bank, blk, page).is_erased = 0;
    if (is_host_data) {
        get_page(bank, blk, page).lpn = lpn;
    } else {
        u8 *data = (u8 *)malloc(VST_BYTES_PER_PAGE * sizeof(u8));
        u8 *addr = (u8 *)dram_addr;
        memcpy(&data[start], addr, length);
        get_page(bank, blk, page).data = data;
    }
}

void vst_copyback_page(u32 const bank, u32 const blk_src, u32 const page_src,
                   u32 const blk_dst, u32 const page_dst)
{
    cnt_flash_cb++;
    #ifdef DEBUG
    //printf("[DEBUG] src = (%u, %u) dst = (%u, %u)\n", 
    //        blk_src, page_src, blk_dst, page_dst);
    #endif
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

    u8 *data = get_page(bank, blk_src, page_src).data;
    if (data != NULL) 
        memcpy(get_page(bank, blk_dst, page_dst).data, 
               get_page(bank, blk_src, page_src).data, 
               VST_BYTES_PER_PAGE);
}

void vst_erase_block(u32 const bank, u32 const blk)
{
    cnt_flash_erase++;
    u32 i;

    for (i = 0; i < VST_PAGES_PER_BLOCK; i++) {
        get_page(bank, blk, i).is_erased = 1;
        u8 *data = get_page(bank, blk, i).data;
        if (data != NULL) {
            free(data);
            get_page(bank, blk, i).data = NULL;
        }
    }
}

/* RAM APIs */
// TODO: assert dram limit
u8 vst_read_dram_8(u64 const addr)
{
    return *(u8 *)addr;
}

u16 vst_read_dram_16(u64 const addr)
{
    assert(!(addr & 1));

    return *(u16 *)addr;
}

u32 vst_read_dram_32(u64 const addr)
{
    assert(!(addr & 3));

    return *(u32 *)addr;
}

void vst_write_dram_8(u64 const addr, u8 const val)
{
    *(u8 *)addr = val;
}

void vst_write_dram_16(u64 const addr, u16 const val)
{
    assert(!(addr & 1));

    *(u16 *)addr = val;
}

void vst_write_dram_32(u64 const addr, u32 const val)
{
    assert(!(addr & 3));

    *(u32 *)addr = val;
}

void vst_set_bit_dram(u64 const base_addr, u32 const bit_offset)
{
    u64 addr = base_addr + bit_offset / 8;
    u32 offset = bit_offset % 8;

    *(u8 *)addr = *(u8 *)addr | (1 << offset);
}

void vst_clr_bit_dram(u64 const base_addr, u32 const bit_offset)
{
    u64 addr = base_addr + bit_offset / 8;
    u32 offset = bit_offset % 8;

    *(u8 *)addr = *(u8 *)addr & ~(1 << offset);
}

u32 vst_tst_bit_dram(u64 const base_addr, u32 const bit_offset)
{
    u64 addr = base_addr + bit_offset / 8;
    u32 offset = bit_offset % 8;

    return (*(u8 *)addr) & (1 << offset);
}

void vst_memcpy(u64 const dst, u64 const src, u32 const len)
{
    memcpy((void *)dst, (void *)src, len);
}

void vst_memset(u64 const addr, u32 const val, u32 const len)
{
    memset((void *)addr, val, len);
}

u32 vst_mem_search_min(u64 const addr, u32 const unit, u32 const size)
{
    assert(unit == 1 || unit == 2 || unit == 4);
    assert(!(addr % unit));
    assert(size != 0);

    u32 i;
    u32 idx = 0;
    if (unit == 1) {
        u8 *vals = (u8 *)addr;
        u8 min = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] < min) {
                min = vals[i];
                idx = i;
            }
        }
    } else if (unit == 2) {
        u16 *vals = (u16 *)addr;
        u16 min = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] < min) {
                min = vals[i];
                idx = i;
            }
        }
    } else {
        u32 *vals = (u32 *)addr;
        u32 min = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] < min) {
                min = vals[i];
                idx = i;
            }
        }
    }
    #ifdef DEBUG
    //printf("[DEBUG] idx = %u\n", idx);
    #endif
    return idx;
}

u32 vst_mem_search_max(u64 const addr, u32 const unit, u32 const size)
{
    assert(!(addr % size));
    assert(unit == 1 || unit == 2 || unit == 4);
    assert(size != 0);

    u32 i;
    u32 idx = 0;
    if (unit == 1) {
        u8 *vals = (u8 *)addr;
        u8 max = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] > max) {
                max = vals[i];
                idx = i;
            }
        }
    } else if (unit == 2) {
        u16 *vals = (u16 *)addr;
        u16 max = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] > max) {
                max = vals[i];
                idx = i;
            }
        }
    } else {
        u32 *vals = (u32 *)addr;
        u32 max = vals[0];
        for (i = 1; i < size; i++) {
            if (vals[i] > max) {
                max = vals[i];
                idx = i;
            }
        }
    }
    return idx;
}

u32 vst_mem_search_equ(u64 const addr, u32 const unit,
                       u32 const size, u32 const val)
{
    //TODO
    return 0;
}

/* init */
void init_vst(void)
{
    init_flash();
    init_dram();
    init_statistic();
}

/* internal routines */
static void init_flash(void)
{
    u32 i, j, k;

    for (i = 0; i < VST_NUM_BANKS; i++) {
        for (j = 0; j < VST_BLOCKS_PER_BANK; j++) {
            for (k = 0; k < VST_PAGES_PER_BLOCK; k++) {
                get_page(i, j, k).data = NULL;
                get_page(i, j, k).is_erased = 1;
            }
        }
    }
    printf("[INFO] Flash initialized\n");
}

static void init_dram(void)
{
    u32 i;

    for (i = 0; i < VST_DRAM_SIZE; i++)
        dram[i] = 0;
    printf("[INFO] DRAM initialized\n");
}

static void init_statistic(void)
{
    byte_write = 0;
    byte_read = 0;
    cnt_flash_read = 0;
    cnt_flash_write = 0;
    cnt_flash_cb = 0;
    cnt_flash_erase = 0;
}

static void report_bug(char *msg)
{
    printf("Bug found: %s\n", msg);
}

