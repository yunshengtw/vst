/**
 * checker.c
 * Authors: Yun-Sheng Chang
 */

#include <stdarg.h>
#include "checker.h"

static int checkable[CHK_MAX];

static void __attribute__((format(printf, 1, 2))) 
violation(const char *fmt, ...)
{
    printf("Bug detected: ");

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

int open_checker(void)
{
    checkable[CHK_LPN_CONSISTENT] = ENABLE_CHK_LPN_CONSISTENT;
    checkable[CHK_NON_SEQ_WRITE] = ENABLE_CHK_NON_SEQ_WRITE;
    checkable[CHK_OVERWRITE] = ENABLE_CHK_OVERWRITE;
    return 0;
}

void close_checker(void)
{
}

void chk_lpn_consistent(flash_page_t *pp, uint32_t lpn)
{
    /* TODO: this might degrade perfomance */
    if (!checkable[CHK_LPN_CONSISTENT])
        return;

    if (!pp->is_erased && pp->lpn != -1 && pp->lpn != lpn) {
        violation("LPN mismatched, issued LPN = %u, stored LPN = %u\n",
                lpn, pp->lpn);
        abort();
    }
}

void chk_non_seq_write(flash_t *flashp, uint32_t bank, uint32_t blk, uint32_t page)
{
    if (!checkable[CHK_NON_SEQ_WRITE])
        return;

    if (page == 0)
        return;
    flash_page_t *pp = &(flashp->banks[bank].blocks[blk].pages[page - 1]);
    if (pp->is_erased) {
        violation("Non-sequential write to bank #%u, blk #%u, page #%u\n",
                bank, blk, page);
        abort();
    }
}

void chk_overwrite(flash_t *flashp, uint32_t bank, uint32_t blk, uint32_t page)
{
    if (!checkable[CHK_OVERWRITE])
        return;

    flash_page_t *pp = &(flashp->banks[bank].blocks[blk].pages[page]);
    if (!pp->is_erased) {
        violation("Directly overwrite to bank #%u , blk #%u, page#%u\n",
                bank, blk, page);
        abort();
    }
}
