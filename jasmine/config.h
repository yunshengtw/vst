/**
 * config.h
 * Authors: Yun-Sheng Chang
 */

#ifndef VST_CONFIG_H
#define VST_CONFIG_H

#include "jasmine.h"

/* ssd configurations */
#define VST_SECTORS_PER_PAGES 
#define VST_PAGES_PER_BLOCK 
#define VST_BLOCKS_PER_BANK 

#define VST_NUM_BANKS 
#define VST_NUM_BLOCKS (VST_NUM_BANKS * VST_BLOCKS_PER_BANK)
#define VST_NUM_PAGES (VST_NUM_BLOCKS * VST_PAGES_PER_BLOCK)
#define VST_NUM_SECTORS (VST_NUM_PAGES * VST_SECTORS_PER_PAGE)

#define VST_BYTES_PER_SECTOR 
#define VST_BYTES_PER_PAGE (VST_SECTORS_PER_PAGE * VST_BYTES_PER_SECTOR)

#define VST_DRAM_BASE 
#define VST_DRAM_SIZE 

#endif // CONFIG_H
