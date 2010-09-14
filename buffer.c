#include "lib.h"
#include "buffer.h"
#include "fat32.h"
#include "io.h"
#include "radix.h"
#include "superblock.h"
#include "dir.h"
#include "page.h"
#include <string.h>

#ifdef DEBUG_MEMORY_USAGE
unsigned int memory_usage = 0;
#endif

struct inode fd_pool[MAX_FD];

void direct_read_sector(void *buf, unsigned int sector)
{
	BUG_ON(buf == NULL);
	read8blocks(buf, sector);
}

void direct_read(void *buf, unsigned int cluster)
{
	unsigned int sector = fat_get_sec(cluster);
	BUG_ON(buf == NULL);
	read8blocks(buf, sector);
}

struct address_space *bread_sector(unsigned int sector)
{
	int create;
	struct address_space *addr;
	if (sector >= RADIX_SIZE * RADIX_SIZE * RADIX_SIZE) {
		addr = alloc_address_space();
		addr->data = alloc_page();
		direct_read_sector(addr->data, sector);
		return addr;
	}
	addr = lookup(sector, &create);
	if (create == NEW_NODE) {
		addr->data = alloc_page();
		direct_read_sector(addr->data, sector);
		return addr;
	} else { // find_node
		return addr;
	}
}

void init_all()
{
#ifdef DEBUG_MEMORY_USAGE
	memory_usage = 0;
#endif

	memset((void*)&fd_pool[0], 0, sizeof(fd_pool));
        init_disk();
        init_superblock();
        init_fat();
        init_address_space();
	init_radix_allocator();
        init_radix_tree();
}

void test_direct_read()
{
	unsigned char *buf = alloc_page();
	direct_read(buf, 2);
}
