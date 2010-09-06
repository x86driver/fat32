#ifndef _MM_H
#define _MM_H

struct address_space {
        unsigned int cluster;
        void *data;
};

extern struct address_space *address_space_array;
extern unsigned int address_space_index;

void init_address_space();

/* alloc_page 分配一個新的頁面給 address_space
 * 先假設 address_space 一開始就先 malloc() 一大塊給他
 */

static inline struct address_space *alloc_address_space()
{
        return address_space_array + address_space_index++;
}

#endif

