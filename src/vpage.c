/**
 * vpage.c
 * Authors: Yun-Sheng Chang
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "config.h"
#include "vpage.h"

/**
 * VST prohibits saving both host data and metadata on the same page.
 */
void tag_page(vpage_t *pp)
{
    if (pp->tagged)
        return;

    for (int i = 0; i < VST_SECTORS_PER_PAGE; i++)
        pp->lbas[i] = -1;
    pp->tagged = 1;
}

void untag_page(vpage_t *pp)
{
    pp->tagged = 0;
}

void vpage_init(vpage_t *pp, uint8_t *data)
{
    pp->tagged = 0;
    pp->data = data;
}

void vpage_copy(vpage_t *dst, vpage_t *src, uint32_t sect, uint32_t n_sect)
{
    assert(sect + n_sect <= VST_SECTORS_PER_PAGE);

    if (src->tagged == 1) {
        /* host data */
        tag_page(dst);
        memcpy(&dst->lbas[sect], &src->lbas[sect], n_sect * sizeof(uint32_t));
    } else {
        /* metadata */
        untag_page(dst);
        if (dst->data == NULL)
            dst->data = (uint8_t *)malloc(VST_BYTES_PER_PAGE * sizeof(uint8_t));
        uint32_t start, length;
        start = sect * VST_BYTES_PER_SECTOR;
        length = n_sect * VST_BYTES_PER_SECTOR;
        if (src->data == NULL)
            /* only flash page may be NULL */
            memset(&dst->data[start], 0xff, length);
        else
            memcpy(&dst->data[start], &src->data[start], length);
    }
}

/* Should only be called by vflash */
void vpage_free(vpage_t *pp)
{
    pp->tagged = 0;
    free(pp->data);
    pp->data = NULL;
    /* dont need to reset lbas as it will be done when the page is tagged */
}
