#include "lib.h"
#include "buffer.h"
#include "fat32.h"
#include "io.h"

extern unsigned char *buf;

/* direct_read() 用來直接讀取某個 cluster
 * 參數是 cluster, 但他會根據 partition
 * 算出要從哪個 block 開始讀, 一次都讀 8 blocks
 * 目的地則是放在 buf
 */

void direct_read(void *buf, unsigned int cluster)
{
	unsigned int sector = fat_get_sec(cluster);
	buf = alloc_page();
	// read disk to buf
	// 這裡做類似 read_clus 的事情
	read8blocks(buf, sector);
}

/* 此函式會將讀到的資料做 buffer
 * 專門給讀 FAT table 以及 DIR entry 專用
 * 因為一次只會讀一個 cluster
 */

struct address_space *bread(unsigned int cluster)
{
	int *create;
	struct address_space *addr = lookup(cluster, &create);
	if (create == NEW_NODE) {
		//哈哈!
		direct_read(addr->data, cluster);
		return addr;
	} else { // find_node
		return addr;
	}
}


int main()
{
	init_disk();
	init_superblock();
	init_fat();
	char *ptr;
	direct_read(ptr, 0);
	return 0;
}
