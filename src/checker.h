/**
 * checker.h
 * Authors: Yun-Sheng Chang
 */

#ifndef CHECKER_H
#define CHECKER_H

#include <stdint.h>
#include "vflash.h"
#include "vpage.h"

#define CHK_LPN_CONSISTENT 0
#define CHK_NON_SEQ_WRITE 1
#define CHK_OVERWRITE 2
#define CHK_MAX 32

#define ENABLE_CHK_LPN_CONSISTENT 1
#define ENABLE_CHK_NON_SEQ_WRITE 0
#define ENABLE_CHK_OVERWRITE 0

int open_checker(void);
void close_checker(void);
void chk_lpn_consistent(vpage_t *pp, uint32_t lba, uint32_t sect, uint32_t n_sect, uint8_t *vers);
void chk_non_seq_write(flash_t *flashp, uint32_t bank, uint32_t blk, uint32_t page);
void chk_overwrite(flash_t *flashp, uint32_t bank, uint32_t blk, uint32_t page);

#endif // CHECKER_H
