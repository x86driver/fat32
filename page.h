#ifndef _PAGE_H
#define _PAGE_H

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_MEMORY_USAGE
extern unsigned int memory_usage;
#endif

static inline void *page_malloc(size_t size)
{
#ifdef DEBUG_MEMORY_USAGE
	memory_usage += size;
#endif

#ifdef DEBUG_MALLOC_POISON
	void *ptr = malloc(size);
	memset(ptr, 0x55, size);
	return ptr;
#else
	return malloc(size);
#endif
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

