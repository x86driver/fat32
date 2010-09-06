#ifndef _PAGE_H
#define _PAGE_H

#include <stdlib.h>

static inline void *page_malloc(size_t size)
{
	return malloc(size);
}

static inline void *any_malloc(size_t size)
{
	return page_malloc(size);
}

static inline void *alloc_page()
{
	return page_malloc(4096);
}

#endif

