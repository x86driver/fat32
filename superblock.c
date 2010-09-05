#include "fat32.h"

void init_superblock()
{
	sb = (struct super_block*)malloc(sizeof(struct super_block));
	sb->dir_clus = 2;
}
