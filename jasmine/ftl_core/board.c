/**
 * board.c
 */

#include "ftl.h"

void build_bad_blk_list(void)
{
    #ifndef VST
    UINT32 bank, num_entries, result, vblk_offset;
    scan_list_t* scan_list = (scan_list_t*) TEMP_BUF_ADDR;

    mem_set_dram(BAD_BLK_BMP_ADDR, 0, BAD_BLK_BMP_BYTES);

    disable_irq();

    flash_clear_irq();

    for (bank = 0; bank < NUM_BANKS; bank++) {
        SETREG(FCP_CMD, FC_COL_ROW_READ_OUT);
        SETREG(FCP_BANK, REAL_BANK(bank));
        SETREG(FCP_OPTION, FO_E);
        SETREG(FCP_DMA_ADDR, (UINT32) scan_list);
        SETREG(FCP_DMA_CNT, SCAN_LIST_SIZE);
        SETREG(FCP_COL, 0);
        SETREG(FCP_ROW_L(bank), SCAN_LIST_PAGE_OFFSET);
        SETREG(FCP_ROW_H(bank), SCAN_LIST_PAGE_OFFSET);

        SETREG(FCP_ISSUE, 0);
        while ((GETREG(WR_STAT) & 0x00000001) != 0);
        while (BSP_FSM(bank) != BANK_IDLE);

        num_entries = 0;
        result = OK;

        if (BSP_INTR(bank) & FIRQ_DATA_CORRUPT) {
            result = FAIL;
        } else
        {
            UINT32 i;

            num_entries = read_dram_16(&(scan_list->num_entries));

            if (num_entries > SCAN_LIST_ITEMS) {
                result = FAIL;
            } else {
                for (i = 0; i < num_entries; i++) {
                    UINT16 entry = read_dram_16(scan_list->list + i);
                    UINT16 pblk_offset = entry & 0x7FFF;
                    if (pblk_offset == 0 || pblk_offset >= PBLKS_PER_BANK) {
                        #if OPTION_REDUCED_CAPACITY == FALSE
                        result = FAIL;
                        #endif
                    } else {
                        write_dram_16(scan_list->list + i, pblk_offset);
                    }
                }
            }
        }

        if (result == FAIL)
            num_entries = 0;  // We cannot trust this scan list. Perhaps a software bug.
        else
            write_dram_16(&(scan_list->num_entries), 0);

        g_bad_blk_count[bank] = 0;

        for (vblk_offset = 1; vblk_offset < VBLKS_PER_BANK; vblk_offset++) {
            BOOL32 bad = FALSE;

            #if OPTION_2_PLANE
            UINT32 pblk_offset;

            pblk_offset = vblk_offset * NUM_PLANES;

            // fix bug@jasmine v.1.1.0
            if (mem_search_equ_dram(scan_list, sizeof(UINT16), num_entries + 1, pblk_offset) < num_entries + 1)
                bad = TRUE;

            pblk_offset = vblk_offset * NUM_PLANES + 1;

            // fix bug@jasmine v.1.1.0
            if (mem_search_equ_dram(scan_list, sizeof(UINT16), num_entries + 1, pblk_offset) < num_entries + 1)
                bad = TRUE;
            #else
            // fix bug@jasmine v.1.1.0
            if (mem_search_equ_dram(scan_list, sizeof(UINT16), num_entries + 1, vblk_offset) < num_entries + 1)
                bad = TRUE;
            #endif

            if (bad) {
                g_bad_blk_count[bank]++;
                set_bit_dram(BAD_BLK_BMP_ADDR + bank*(VBLKS_PER_BANK/8 + 1), vblk_offset);
            }
        }
    }
    #endif
}

void wait_rdbuf_free(UINT32 bufid)
{
    #ifndef VST
    #if OPTION_FTL_TEST == 0
    while (bufid == GETREG(SATA_RBUF_PTR));
    #endif
    #endif
}

void set_bm_read_limit(UINT32 bufid)
{
    #ifndef VST
    SETREG(BM_STACK_RDSET, bufid);
    SETREG(BM_STACK_RESET, 0x02);
    #endif
}

/* This function writes a format mark to a page at bank #0, block #0 */
void write_format_mark(void)
{
    #ifndef VST
    #ifdef __GNUC__
    extern UINT32 size_of_firmware_image;
    UINT32 firmware_image_pages = (((UINT32) (&size_of_firmware_image)) +
            BYTES_PER_FW_PAGE - 1) / BYTES_PER_FW_PAGE;
    #else
    extern UINT32 Image$$ER_CODE$$RO$$Length;
    extern UINT32 Image$$ER_RW$$RW$$Length;
    UINT32 firmware_image_bytes = ((UINT32) &Image$$ER_CODE$$RO$$Length) +
            ((UINT32) &Image$$ER_RW$$RW$$Length);
    UINT32 firmware_image_pages =
            (firmware_image_bytes + BYTES_PER_FW_PAGE - 1) / BYTES_PER_FW_PAGE;
    #endif

    UINT32 format_mark_page_offset = FW_PAGE_OFFSET + firmware_image_pages;

    mem_set_dram(FTL_BUF_ADDR, 0, BYTES_PER_SECTOR);

    SETREG(FCP_CMD, FC_COL_ROW_IN_PROG);
    SETREG(FCP_BANK, REAL_BANK(0));
    SETREG(FCP_OPTION, FO_E | FO_B_W_DRDY);
    SETREG(FCP_DMA_ADDR, FTL_BUF_ADDR);     // DRAM -> flash
    SETREG(FCP_DMA_CNT, BYTES_PER_SECTOR);
    SETREG(FCP_COL, 0);
    SETREG(FCP_ROW_L(0), format_mark_page_offset);
    SETREG(FCP_ROW_H(0), format_mark_page_offset);

    /*
     * At this point, we do not have to check Waiting Room status
     * before issuing a command, because we have waited for all the
     * banks to become idle before returning from format().
     */
    SETREG(FCP_ISSUE, 0);

    /* wait for the FC_COL_ROW_IN_PROG command to be accepted by bank #0 */
    while ((GETREG(WR_STAT) & 0x00000001) != 0);

    /* wait until bank #0 finishes the write operation */
    while (BSP_FSM(0) != BANK_IDLE);
    #endif
}

/* 
 * This function reads a flash page from (bank #0, block #0)
 * in order to check whether the SSD is formatted or not.
 */
BOOL32 check_format_mark(void)
{
    #ifndef VST
    #ifdef __GNUC__
    extern UINT32 size_of_firmware_image;
    UINT32 firmware_image_pages = (((UINT32) (&size_of_firmware_image)) +
        BYTES_PER_FW_PAGE - 1) / BYTES_PER_FW_PAGE;
    #else
    extern UINT32 Image$$ER_CODE$$RO$$Length;
    extern UINT32 Image$$ER_RW$$RW$$Length;
    UINT32 firmware_image_bytes = ((UINT32) &Image$$ER_CODE$$RO$$Length) +
            ((UINT32) &Image$$ER_RW$$RW$$Length);
    UINT32 firmware_image_pages =
            (firmware_image_bytes + BYTES_PER_FW_PAGE - 1) / BYTES_PER_FW_PAGE;
    #endif

    UINT32 format_mark_page_offset = FW_PAGE_OFFSET + firmware_image_pages;
    UINT32 temp;

    /* clear any flash interrupt flags that might have been set */
    flash_clear_irq();

    SETREG(FCP_CMD, FC_COL_ROW_READ_OUT);
    SETREG(FCP_BANK, REAL_BANK(0));
    SETREG(FCP_OPTION, FO_E);
    SETREG(FCP_DMA_ADDR, FTL_BUF_ADDR);     // flash -> DRAM
    SETREG(FCP_DMA_CNT, BYTES_PER_SECTOR);
    SETREG(FCP_COL, 0);
    SETREG(FCP_ROW_L(0), format_mark_page_offset);
    SETREG(FCP_ROW_H(0), format_mark_page_offset);

    /*
     * At this point, we do not have to check Waiting Room status
     * before issuing a command, because scan list loading has been
     * completed just before this function is called.
     */
    SETREG(FCP_ISSUE, 0);

    /* wait for the FC_COL_ROW_READ_OUT command to be accepted by bank #0 */
    while ((GETREG(WR_STAT) & 0x00000001) != 0);

    /* wait until bank #0 finishes the read operation */
    while (BSP_FSM(0) != BANK_IDLE);

    /* Now that the read operation is complete, we can check interrupt flags */
    temp = BSP_INTR(0) & FIRQ_ALL_FF;
    /* clear interrupt flags */
    CLR_BSP_INTR(0, 0xFF);

    if (temp != 0)
        /* the page contains all-0xFF (the format mark does not exist) */
        return FALSE;
    else
        /* the page contains sth other than 0xFF (must be the format mark) */
        return TRUE;
    #else
        return TRUE;
    #endif
}

void fill_buf_ff(UINT32 addr, UINT32 sect, UINT32 n_sect)
{
    #ifdef VST
    omit_next_dram_op();
    #endif
    mem_set_dram(addr + sect * BYTES_PER_SECTOR, 0xffffffff, n_sect * BYTES_PER_SECTOR);
}

void set_intr_mask(UINT32 flags)
{
    #ifndef VST
    SETREG(INTR_MASK, flags);
    #endif
}

void set_fconf_pause(UINT32 flags)
{
    #ifndef VST
    SETREG(FCONF_PAUSE, flags);
    #endif
}
