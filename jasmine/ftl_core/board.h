/**
 * board.h
 */

#ifndef BOARD_H
#define BOARD_H

void build_bad_blk_list(void);
void wait_rdbuf_free(UINT32 bufid);
void set_bm_read_limit(UINT32 bufid);
void write_format_mark(void);
BOOL32 check_format_mark(void);
void fill_buf_ff(UINT32 addr, UINT32 sect, UINT32 n_sect);
void set_intr_mask(UINT32 flags);
void set_fconf_pause(UINT32 flags);

#endif // BOARD_H
