#include "fat32.h"
#include "superblock.h"
#include "page.h"

struct super_block *sb;

void init_superblock()
{
	sb = (struct super_block*)any_malloc(sizeof(struct super_block));
	sb->dir_clus = 2;
}
