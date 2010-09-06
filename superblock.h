#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

struct super_block {
	unsigned int dir_clus;
};

void init_superblock();

#endif

