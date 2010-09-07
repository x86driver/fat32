#ifndef _MM_H
#define _MM_H

#include "lib.h"

#define RADIX_THRESHOLD 4
#define CACHE_ENTRY 8192        /* 8192 * 4096 = 32M */
#define RADIX_SIZE_LV2 CACHE_ENTRY

struct address_space {
        unsigned int cluster;
        void *data;
};

extern struct address_space *address_space_array;
extern unsigned int address_space_index;

void init_address_space();
struct radix_tree *alloc_radix_tree();
void init_radix_allocator();

/* alloc_page 分配一個新的頁面給 address_space
 * 先假設 address_space 一開始就先 malloc() 一大塊給他
 */

static inline struct address_space *alloc_address_space()
{
	BUG_ON(address_space_index >= CACHE_ENTRY);
        return address_space_array + address_space_index++;
}

#endif

