#include "fat32.h"

void init_superblock()
{
	sb->dir_clus = 2;
	init_fat();
}
