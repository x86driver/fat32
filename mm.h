#ifndef _MM_H
#define _MM_H

#include "page.h"

struct address_space {
	unsigned int cluster;
	void *data;
};

static struct address_space *address_space_array;
static unsigned int address_space_index;

void init_address_space()
{
	//數量是128*128 = 16384
        address_space_array = (struct address_space*)page_malloc(16384 * sizeof(struct address_space));
        address_space_index = 0;
}

/* alloc_page 分配一個新的頁面給 address_space
 * 先假設 address_space 一開始就先 malloc() 一大塊給他
 */

static inline struct address_space *alloc_address_space()
{
//	return address_space_array[address_space_index++];
	return address_space_array + address_space_index++;
}

#endif

