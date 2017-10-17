/**
 * port.c
 * Vanilla SSD porting
 * Authors: Yun-Sheng Chang
 */

#include "vst.h"

void flash_read(uint32_t bank, uint32_t block, uint32_t page, uint32_t sect,
                uint32_t n_sect, uint32_t buf, uint32_t lpn,
                uint32_t issue_flag)
{
    uint8_t is_host = (lpn == -1) ? 0 : 1;
    vst_read_page(bank, block, page, sect, n_sect,
                  (uintptr_t)buf, lpn, is_host);
}

void flash_program(uint32_t bank, uint32_t block, uint32_t page, uint32_t sect,
                   uint32_t n_sect, uint32_t buf, uint32_t lpn,
                   uint32_t issue_flag)
{
    uint8_t is_host = (lpn == -1) ? 0 : 1;
    vst_write_page(bank, block, page, sect, n_sect,
                   (uintptr_t)buf, lpn, is_host);
}

void flash_copyback(uint32_t bank, uint32_t dst_blk, uint32_t dst_page,
                    uint32_t src_blk, uint32_t src_page, uint32_t issue_flag)
{
    vst_copyback_page(bank, src_blk, src_page, 
                      dst_blk, dst_page);
}

void flash_erase(uint32_t bank, uint32_t blk, uint32_t issue_flag)
{
    vst_erase_block(bank, blk);
}
