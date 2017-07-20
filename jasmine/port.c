/**
 * port.c
 * Authors: Yun-Sheng Chang
 */

#include "ftl.h"
#include "vst.h"

/* VST tags */
UINT8 g_real_dram_op;
UINT8 g_real_flash_op;
UINT32 g_lpn;

/* FTL metadata */
extern UINT32 g_ftl_read_buf_id;
extern UINT32 g_ftl_write_buf_id;

/* VST tag operations */
void real_dram_op(void)
{
    g_real_dram_op = 1;
}

void fake_dram_write(void)
{
    g_real_dram_op = 0;
}

void real_flash_op(void)
{
    g_real_flash_op = 1;
}

void fake_flash_write(UINT32 lpn)
{
    g_real_flash_op = 0;
    g_lpn = lpn;
}

/* flash wrappers */
void nand_page_read(UINT32 const bank, UINT32 const vblock, 
                    UINT32 const page_num, UINT32 const buf_addr)
{
    vst_read_page(bank, vblock, page_num, 0, SECTORS_PER_PAGE, (UINT64)buf_addr,
                  g_lpn, g_real_flash_op);
}

void nand_page_ptread(UINT32 const bank, UINT32 const vblock, 
                      UINT32 const page_num, UINT32 const sect_offset, 
                      UINT32 const num_sectors, UINT32 const buf_addr, 
                      UINT32 const issue_flag)
{
    vst_read_page(bank, vblock, page_num, sect_offset, num_sectors,
                  (UINT64)buf_addr, g_lpn, g_real_flash_op);
}

void nand_page_read_to_host(UINT32 const bank, UINT32 const vblock,
                           UINT32 const page_num)
{
    // TODO
}

void nand_page_ptread_to_host(UINT32 const bank, UINT32 const vblock, 
                              UINT32 const page_num, UINT32 const sect_offset,
                              UINT32 const num_sectors)
{
    vst_read_page(bank, vblock, page_num, sect_offset, num_sectors, 
                  (UINT64)RD_BUF_PTR(g_ftl_read_buf_id), 
                  g_lpn, g_real_flash_op);
}

void nand_page_program(UINT32 const bank, UINT32 const vblock, 
                       UINT32 const page_num, UINT32 const buf_addr)
{
    // TODO
}

void nand_page_ptprogram(UINT32 const bank, UINT32 const vblock, 
                         UINT32 const page_num, UINT32 const sect_offset, 
                         UINT32 const num_sectors, UINT32 const buf_addr)
{
    vst_write_page(bank, vblock, page_num, sect_offset, num_sectors,
                   (UINT64)buf_addr, g_lpn, g_real_flash_op);
}

void nand_page_program_from_host(UINT32 const bank, UINT32 const vblock, 
                                 UINT32 const page_num)
{
    // TODO
}

void nand_page_ptprogram_from_host(UINT32 const bank, UINT32 const vblock, 
                                   UINT32 const page_num, 
                                   UINT32 const sect_offset, 
                                   UINT32 const num_sectors)
{
    vst_write_page(bank, vblock, page_num, sect_offset, num_sectors,
                   (UINT64)WR_BUF_PTR(g_ftl_write_buf_id), 
                   g_lpn, g_real_flash_op);
}

void nand_page_copyback(UINT32 const bank,
                        UINT32 const src_vblock, UINT32 const src_page,
                        UINT32 const dst_vblock, UINT32 const dst_page)
{
    vst_copyback_page(bank, src_vblock, src_page, 
                      dst_vblock, dst_page);
}

void nand_block_erase(UINT32 const bank, UINT32 const vblock)
{
    vst_erase_block(bank, vblock);
}

/* dummy functions */
UINT32 disable_irq(void)
{
}

void enable_irq(void)
{
}

UINT32 disable_fiq(void)
{
}

void enable_fiq(void)
{
}

void flash_clear_irq(void)
{
}

void flash_finish(void)
{
}

void led(BOOL32 on)
{
}

void led_blink(void)
{
}

