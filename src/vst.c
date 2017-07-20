/**
 * vst.c
 * Authors: Yun-Sheng Chang
 */

#include "vst.h"

// TODO: assertions

/* emulated DRAM and Flash memory */
static u8 __attribute__((section (".dram"))) dram[VST_SIZE_DRAM];
static flash_t flash;

/* statistic results */


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
    if (!get_page(bank, blk_src, page_src).is_erased) {
        char msg[200];
        sprintf(msg, "in-place write to dirty page, "
            "bank = %u " 
            "block = %u "
            "page = %u", 
            bank, blk_dst, page_dst);
        report_bug(msg);
        assert(0);
    }

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
    u32 i;

    for (i = 0; i < VST_PAGES_PER_BLK; i++) {
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
// TODO: consistent name, dram or ext
u8 vst_read_ext_8(u64 const addr)
{
    return *(u8 *)addr;
}

u16 vst_read_ext_16(u64 const addr)
{
    assert(!(addr & 1));

    return *(u16 *)addr;
}

u32 vst_read_ext_32(u64 const addr)
{
    assert(!(addr & 3));

    return *(u32 *)addr;
}

void vst_write_ext_8(u64 const addr, u8 const val)
{
    *(u8 *)addr = val;
}

void vst_write_ext_16(u64 const addr, u16 const val)
{
    assert(!(addr & 1));

    *(u16 *)addr = val;
}

void vst_write_ext_32(u64 const addr, u32 const val)
{
    assert(!(addr & 3));

    *(u32 *)addr = val;
}

void vst_set_bit_dram(u64 const base_addr, u32 const bit_offset)
{
    u64 addr = base_addr + bit_offset / 8;
    addr = addr & ~0x3;
    u32 offset = bit_offset % 32;

    *(u32 *)addr = *(u32 *)addr | (1 << offset);
}

void vst_clr_bit_dram(u64 const base_addr, u32 const bit_offset)
{
    u64 addr = base_addr + bit_offset / 8;
    addr = addr & ~0x3;
    u32 offset = bit_offset % 32;

    *(u32 *)addr = *(u32 *)addr & ~(1 << offset);
}

u32 vst_tst_bit_dram(u64 const base_addr, u32 const bit_offset)
{
    u64 addr = base_addr + bit_offset / 8;
    addr = addr & ~0x3;
    u32 offset = bit_offset % 32;

    return (*(u32 *)addr) & (1 << offset);
}

void vst_mem_copy(u64 const dst, u64 const src, u32 const len)
{
    memcpy((void *)dst, (void *)src, len);
}

void vst_mem_set_sram(u64 const addr, u32 const val, u32 const len)
{
    memset((void *)addr, val, len);
}

void vst_mem_set_dram(u64 const addr, u32 const val, u32 const len)
{
    // TODO: dram boundary check

    memset((void *)addr, val, len);
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
        for (j = 0; j < VST_BLKS_PER_BANK; j++) {
            for (k = 0; k < VST_PAGES_PER_BLK; k++) {
                get_page(i, j, k).data = NULL;
                get_page(i, j, k).is_erased = 1;
            }
        }
    }
}

static void init_dram(void)
{
    u32 i;

    for (i = 0; i < VST_SIZE_DRAM; i++)
        dram[i] = 0;
}

static void init_statistic(void)
{
}

static void report_bug(char *msg)
{
    printf("Bug found: %s\n", msg);
}

