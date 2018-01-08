/**
 * vpage.h
 * Authors: Yun-Sheng Chang
 */

#ifndef VPAGE_H
#define VPAGE_H

#include <stdint.h>
#include "config.h"

typedef struct {
    int tagged;
    uint8_t *data;
    uint32_t lbas[VST_SECTORS_PER_PAGE];
} vpage_t;

void tag_page(vpage_t *pp);
void untag_page(vpage_t *pp);
void vpage_init(vpage_t *pp, uint8_t *data);
void vpage_copy(vpage_t *dst, vpage_t *src, uint32_t sect, uint32_t n_sect);
void vpage_free(vpage_t *pp);

#endif // VPAGE_H
