/**
 * ftl.c
 * Core FTL
 * Features:
 * 1. page-level mapping
 * 2. garbage collection
 * 3. write buffer and cache flushing
 */

#include <assert.h>
#include "ftl.h"
#include "board.h"

/* TODO: bad name */
#define VC_MAX 0xCDCD
#define ROUND_UP(x, a) (((x) + (a) - 1) / (a) * (a))
#define ROUND_DOWN(x, a) ((x) / (a) * (a))
#define RSV_VBLKS_PER_BANK 

static void load_metadata(void);
static void save_metadata(void);
static BOOL32 is_bad_block(UINT32 bank, UINT32 blk);
static void sanity_check(void);
static void format(void);
static UINT32 get_ppn(UINT32 const lpn);
static void set_ppn(UINT32 const lpn, UINT32 const ppn);
static UINT32 get_lpn(UINT32 const bank, UINT32 const page);
static void set_lpn(UINT32 const bank, UINT32 const page, 
                    UINT32 const lpn);
static UINT16 get_vcount(UINT32 const bank, UINT32 const blk);
static void set_vcount(UINT32 const bank, UINT32 const blk, 
                       UINT16 vcount);
static UINT32 get_active_ppn(UINT32 const bank);
static UINT32 get_last_write_ppn(UINT32 const bank);
static void set_last_write_ppn(UINT32 const bank, UINT32 const ppn);
static UINT32 get_pgmap_ppn(UINT32 const bank);
static void set_pgmap_ppn(UINT32 const bank, UINT32 const ppn);
static UINT32 garbage_collection(UINT32 const bank);
static UINT32 get_free_blk_cnt(UINT32 const bank);
static void set_free_blk_cnt(UINT32 const bank, UINT32 const cnt);
static void inc_free_blk_cnt(UINT32 const bank);
static void dec_free_blk_cnt(UINT32 const bank);
static UINT32 get_bad_blk_cnt(UINT32 const bank);
static void set_bad_blk_cnt(UINT32 const bank, UINT32 const cnt);
static UINT32 get_vt_blk(UINT32 const bank);
static UINT32 get_gc_blk(UINT32 const bank);
static void set_gc_blk(UINT32 const bank, UINT32 const blk);
static UINT32 reach_gc_threshold(UINT32 const bank);

typedef struct {
    UINT32 free_blk_cnt;
    UINT32 bad_blk_cnt;
    UINT32 last_write_ppn;
    UINT32 pgmap_ppn;
    UINT32 gc_blk;
    UINT32 lpns[PAGES_PER_VBLK];
} bank_state_t;

static bank_state_t state_of_banks[NUM_BANKS];
UINT32 g_ftl_read_buf_id, g_ftl_write_buf_id;

void ftl_open(void)
{
    uart_printf("ftl_open() starts\n");
    sanity_check();

    build_bad_blk_list();

    /* TODO: currently, this FTL cant save any data after reboot */
    if (TRUE)
        format();
    else
        load_metadata();

    g_ftl_read_buf_id = 0;
    g_ftl_write_buf_id = 0;

    flash_clear_irq();

    set_intr_mask(FIRQ_DATA_CORRUPT | FIRQ_BADBLK_L | FIRQ_BADBLK_H);
    set_fconf_pause(FIRQ_DATA_CORRUPT | FIRQ_BADBLK_L | FIRQ_BADBLK_H);

    enable_irq();

    uart_printf("ftl_open() ends\n");
}

#define get_bank(lpn) ((lpn) % NUM_BANKS)
void ftl_read(UINT32 const lba, UINT32 const n_sect)
{
    UINT32 remain_sect, base_sect, cnt_sect;
    UINT32 lpn, ppn;
    UINT32 bank;

    remain_sect = n_sect;
    lpn = lba / SECTORS_PER_PAGE;
    base_sect = lba % SECTORS_PER_PAGE;

    while (remain_sect != 0) {
        if (base_sect + remain_sect < SECTORS_PER_PAGE)
            cnt_sect = remain_sect;
        else
            cnt_sect = SECTORS_PER_PAGE - base_sect;

        bank = get_bank(lpn);
        ppn = get_ppn(lpn);

        if (ppn != 0) {
            nand_page_ptread_to_host(bank, ppn / PAGES_PER_VBLK, ppn % PAGES_PER_VBLK,
                    base_sect, cnt_sect);
        } else {
            /* try to read a logical page that has never been written to */
            UINT32 next_read_buf_id = (g_ftl_read_buf_id + 1) % NUM_RD_BUFFERS;
            wait_rdbuf_free(next_read_buf_id);
            fill_buf_ff(RD_BUF_PTR(g_ftl_read_buf_id), base_sect, cnt_sect);
            flash_finish();
            set_bm_read_limit(next_read_buf_id);
            g_ftl_read_buf_id = next_read_buf_id;
        }

        base_sect = 0;
        remain_sect -= cnt_sect;
        lpn++;
    }
}

void ftl_write(UINT32 const lba, UINT32 const n_sect)
{
    UINT32 remain_sect, base_sect, cnt_sect;
    UINT32 lpn;
    UINT32 old_ppn, new_ppn;
    UINT32 blk, page;
    UINT32 bank;
    BOOL32 is_new;

    remain_sect = n_sect;
    lpn = lba / SECTORS_PER_PAGE;
    base_sect = lba % SECTORS_PER_PAGE;

    while (remain_sect != 0) {
        if (base_sect + remain_sect < SECTORS_PER_PAGE)
            cnt_sect = remain_sect;
        else
            cnt_sect = SECTORS_PER_PAGE - base_sect;

        bank = get_bank(lpn);
        new_ppn = get_active_ppn(bank);
        old_ppn = get_ppn(lpn);

        is_new = TRUE;
        if (old_ppn != 0) {
            blk = old_ppn / PAGES_PER_VBLK;
            page = old_ppn % PAGES_PER_VBLK;

            if (cnt_sect != SECTORS_PER_PAGE) {
                /**
                 * this is an update operation AND not a full page write,
                 * do read-modify-write
                 */
                if (cnt_sect <= 8 && base_sect != 0) {
                    nand_page_read(bank, blk, page, FTL_BUF(bank));
                    if (base_sect != 0)
                        /* left hole */
                        mem_copy(WR_BUF_PTR(g_ftl_write_buf_id), 
                                FTL_BUF(bank),
                                base_sect * BYTES_PER_SECTOR);
                    if (base_sect + cnt_sect < SECTORS_PER_PAGE) {
                        /* right hole */
                        UINT32 offset = (base_sect + cnt_sect) * BYTES_PER_SECTOR;
                        mem_copy(WR_BUF_PTR(g_ftl_write_buf_id) + offset, 
                                FTL_BUF(bank) + offset,
                                BYTES_PER_PAGE - offset);
                    }
                } else {
                    if (base_sect != 0)
                        nand_page_ptread(bank, blk, page, 0, base_sect,
                                WR_BUF_PTR(g_ftl_write_buf_id),
                                RETURN_ON_ISSUE);
                    if (base_sect + cnt_sect < SECTORS_PER_PAGE)
                        nand_page_ptread(bank, blk, page, base_sect + cnt_sect,
                                SECTORS_PER_PAGE - (base_sect + cnt_sect),
                                WR_BUF_PTR(g_ftl_write_buf_id),
                                RETURN_ON_ISSUE);
                }
            }

            set_vcount(bank, blk, get_vcount(bank, blk) - 1);
            is_new = FALSE;
        }

        blk = new_ppn / PAGES_PER_VBLK;
        page = new_ppn % PAGES_PER_VBLK;

        if (is_new)
            nand_page_ptprogram_from_host(bank, blk, page, base_sect, cnt_sect);
        else
            nand_page_program_from_host(bank, blk, page);

        /* update metadata */
        set_lpn(bank, page, lpn);
        set_ppn(lpn, new_ppn);
        set_vcount(bank, blk, get_vcount(bank, blk) + 1);

        base_sect = 0;
        remain_sect -= cnt_sect;
        lpn++;
    }
}

void ftl_flush(void)
{
}

/* TODO */
static void load_metadata(void)
{
}

/* TODO */
static void save_metadata(void)
{
}

static BOOL32 is_bad_block(UINT32 bank, UINT32 blk)
{
    return tst_bit_dram(BAD_BLK_BMP_ADDR +
            bank * (VBLKS_PER_BANK / 8 + 1), blk);
}

/* TODO */
static void sanity_check(void)
{
}

static void format(void)
{
    UINT32 bank, blk;

    /* erase all blocks */
    for (bank = 2; bank < NUM_BANKS; bank++) {
        for (blk = 0; blk < VBLKS_PER_BANK; blk++) {
            if (!is_bad_block(bank, blk)) {
                nand_block_erase(bank, blk);
                set_vcount(bank, blk, 0);
            } else {
                set_vcount(bank, blk, VC_MAX);
            }
        }
    }

    /* init dram */
    mem_set_dram(PAGE_MAP_ADDR, 0, PAGE_MAP_BYTES);
    mem_set_dram(VCOUNT_ADDR, 0, VCOUNT_BYTES);

    /* init sram */
    for (bank = 0; bank < NUM_BANKS; bank++) {
        set_free_blk_cnt(bank, VBLKS_PER_BANK - get_bad_blk_cnt(bank));

        /* reserve bad block bitmap and misc. block */
        set_vcount(bank, 0, VC_MAX);
        dec_free_blk_cnt(bank);
        set_vcount(bank, 1, VC_MAX);
        dec_free_blk_cnt(bank);

        blk = 2;
        BOOL32 found = 0;
        do {
            /* reserve map block */
            if (!is_bad_block(bank, blk)) {
                set_vcount(bank, blk, VC_MAX);
                set_pgmap_ppn(bank, blk * PAGES_PER_VBLK);
                dec_free_blk_cnt(bank);
                found = 1;
            }
            blk++;
        } while (!found);

        found = 0;
        do {
            /* assign a free block for gc */
            if (!is_bad_block(bank, blk)) {
                set_vcount(bank, blk, VC_MAX);
                set_gc_blk(bank, blk);
                found = 1;
            }
            blk++;
        } while (!found);

        found = 0;
        do {
            /* assign a free block for the first write */
            if (!is_bad_block(bank, blk)) {
                set_last_write_ppn(bank, blk * PAGES_PER_VBLK - 1);
                found = 1;
            }
            blk++;
        } while (!found);
    }

    save_metadata();

    write_format_mark();
}

static UINT32 get_ppn(UINT32 const lpn)
{
    return read_dram_32(PAGE_MAP_ADDR + lpn * sizeof(UINT32));
}

static void set_ppn(UINT32 const lpn, UINT32 const ppn)
{
    write_dram_32(PAGE_MAP_ADDR + lpn * sizeof(UINT32), ppn);
}

static UINT32 get_lpn(UINT32 const bank, UINT32 const page)
{
    return state_of_banks[bank].lpns[page];
}

static void set_lpn(UINT32 const bank, UINT32 const page, UINT32 const lpn)
{
    state_of_banks[bank].lpns[page] = lpn;
}

static UINT16 get_vcount(UINT32 const bank, UINT32 const blk)
{
    return read_dram_16(VCOUNT_ADDR + ((bank * VBLKS_PER_BANK) + blk) *
            sizeof(UINT16));
}
 
static void set_vcount(UINT32 const bank, UINT32 const blk,
                       UINT16 const vcount)
{
    assert(vcount == VC_MAX || (vcount >= 0 && vcount <= PAGES_PER_VBLK));
    write_dram_16(VCOUNT_ADDR + ((bank * VBLKS_PER_BANK) + blk) *
            sizeof(UINT16), vcount);
}

/**
 * the semantic of "active ppn" is the ppn of the active page, which is
 * a clean page for the nearest write to settle
 */
static UINT32 get_active_ppn(UINT32 const bank)
{
    UINT32 ppn;
    UINT32 blk;

    ppn = get_last_write_ppn(bank);
    blk = ppn / PAGES_PER_VBLK;

    if (ppn % PAGES_PER_VBLK == PAGES_PER_VBLK - 2) {
        /* program p2l table into summary page, and clear in-sram p2l table */
        mem_copy(FTL_BUF(bank), state_of_banks[bank].lpns, 
                sizeof(UINT32) * PAGES_PER_VBLK);
        nand_page_ptprogram(bank, blk, PAGES_PER_VBLK - 1, 0,
                ROUND_UP(sizeof(UINT32) * PAGES_PER_VBLK, BYTES_PER_SECTOR) /
                BYTES_PER_SECTOR, FTL_BUF(bank));

        mem_set_sram(state_of_banks[bank].lpns, 0x00000000, sizeof(UINT32) * 
                PAGES_PER_VBLK);

        dec_free_blk_cnt(bank);
        /* TODO: improve this */
        if (reach_gc_threshold(bank)) {
            /* gc determines active page */
            ppn = garbage_collection(bank);
        } else {
            do {
                blk++;
            } while (get_vcount(bank, blk) == VC_MAX);
            ppn = blk * PAGES_PER_VBLK;
        }
    } else {
        ppn++;
    }

    assert(ppn / PAGES_PER_VBLK < VBLKS_PER_BANK);
    assert(ppn % PAGES_PER_VBLK != PAGES_PER_VBLK - 1);
    set_last_write_ppn(bank, ppn);

    return ppn;
}

static UINT32 get_last_write_ppn(UINT32 const bank)
{
    return state_of_banks[bank].last_write_ppn;
}

static void set_last_write_ppn(UINT32 const bank, UINT32 const ppn)
{
    state_of_banks[bank].last_write_ppn = ppn;
}

static UINT32 get_pgmap_ppn(UINT32 const bank)
{
    return state_of_banks[bank].pgmap_ppn;
}

static void set_pgmap_ppn(UINT32 const bank, UINT32 const ppn)
{
    state_of_banks[bank].pgmap_ppn = ppn;
}

static UINT32 garbage_collection(UINT32 const bank)
{
    UINT32 lpn;
    UINT32 vt_blk, gc_blk;
    UINT32 vt_page, gc_page;
    UINT32 vcount;

    vt_blk = get_vt_blk(bank);
    gc_blk = get_gc_blk(bank);
    assert(get_vcount(bank, gc_blk) == VC_MAX);
    vcount = get_vcount(bank, vt_blk);

    nand_page_ptread(bank, vt_blk, PAGES_PER_VBLK - 1, 0,
            ROUND_UP(sizeof(UINT32) * PAGES_PER_VBLK, BYTES_PER_SECTOR) /
            BYTES_PER_SECTOR, FTL_BUF(bank), RETURN_WHEN_DONE);
    mem_copy(state_of_banks[bank].lpns, FTL_BUF(bank), 
            sizeof(UINT32) * PAGES_PER_VBLK);

    gc_page = 0;
    for (vt_page = 0; vt_page < (PAGES_PER_VBLK - 1); vt_page++) {
        lpn = get_lpn(bank, vt_page);

        if (get_ppn(lpn) == vt_blk * PAGES_PER_VBLK + vt_page) {
            /* valid page found */
            nand_page_copyback(bank, vt_blk, vt_page, gc_blk, gc_page);
            set_ppn(lpn, gc_blk * PAGES_PER_VBLK + gc_page);
            set_lpn(bank, gc_page, lpn);
            gc_page++;
        }
    }
    assert(gc_page == vcount);

    nand_block_erase(bank, vt_blk);

    /* update metadata */
    set_vcount(bank, vt_blk, VC_MAX);
    set_vcount(bank, gc_blk, vcount);
    set_gc_blk(bank, vt_blk);
    inc_free_blk_cnt(bank);

    return gc_blk * PAGES_PER_VBLK + gc_page;
}

static UINT32 get_free_blk_cnt(UINT32 const bank)
{
    return state_of_banks[bank].free_blk_cnt;
}

static void set_free_blk_cnt(UINT32 const bank, UINT32 const cnt)
{
    state_of_banks[bank].free_blk_cnt = cnt;
}

static void inc_free_blk_cnt(UINT32 const bank)
{
    state_of_banks[bank].free_blk_cnt++;
}

static void dec_free_blk_cnt(UINT32 const bank)
{
    state_of_banks[bank].free_blk_cnt--;
}

static UINT32 get_bad_blk_cnt(UINT32 const bank)
{
    return state_of_banks[bank].bad_blk_cnt;
}

static void set_bad_blk_cnt(UINT32 const bank, UINT32 const cnt)
{
    state_of_banks[bank].bad_blk_cnt = cnt;
}

static UINT32 get_vt_blk(UINT32 const bank)
{
    UINT16 *vcounts = (UINT16 *)
            (VCOUNT_ADDR + bank * VBLKS_PER_BANK * sizeof(UINT16));
    UINT32 idx;
    UINT16 val, min;

    min = vcounts[0];
    idx = 0;
    if (!min)
        return 0;
    for (int i = 1; i < VBLKS_PER_BANK; i++) {
        val = vcounts[i];
        if (val < min) {
            if (val == 0)
                return i;
            min = val;
            idx = i;
        }
    }
    return idx;
}

static UINT32 get_gc_blk(UINT32 const bank)
{
    return state_of_banks[bank].gc_blk;
}

static void set_gc_blk(UINT32 const bank, UINT32 const blk)
{
    state_of_banks[bank].gc_blk = blk;
}

static UINT32 reach_gc_threshold(UINT32 const bank)
{
    return (state_of_banks[bank].free_blk_cnt == 1);
}
