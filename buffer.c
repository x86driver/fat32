#include "lib.h"
#include "buffer.h"
#include "fat32.h"
#include "io.h"
#include "radix.h"
#include "superblock.h"
#include "dir.h"
#include "page.h"

extern unsigned char *buf;

void direct_read_sector(void *buf, unsigned int sector)
{
	BUG_ON(buf == NULL);
	read8blocks(buf, sector);
}

/* direct_read() 用來直接讀取某個 cluster
 * 參數是 cluster, 但他會根據 partition
 * 算出要從哪個 block 開始讀, 一次都讀 8 blocks
 * 目的地則是放在 buf
 * 放資料的地方必須由 buf 決定
 * 因為使用者也許會讀連續的一大筆資料
 */
void direct_read(void *buf, unsigned int cluster)
{
	unsigned int sector = fat_get_sec(cluster);
	BUG_ON(buf == NULL);
	// read disk to buf
	// 這裡做類似 read_clus 的事情
	read8blocks(buf, sector);
}

/* 此函式會將讀到的資料做 buffer
 * 專門給讀 FAT table 以及 DIR entry 專用
 * 因為一次只會讀一個 cluster
 * 而且不用管資料讀到哪裡, 因此放資料的地方用 alloc_page()
 */

struct address_space *bread(unsigned int cluster)
{
	int create;
	struct address_space *addr = lookup(cluster, &create);
	if (create == NEW_NODE) {
		addr->data = alloc_page();
		direct_read(addr->data, cluster);
		return addr;
	} else { // find_node
		return addr;
	}
}

void init_all()
{
        init_disk();
        init_superblock();
        init_fat();
        init_address_space();
        init_radix_tree();
}

void test_direct_read()
{
	unsigned char *buf = alloc_page();
	direct_read(buf, 2);
//	dump(buf, 0, 512);
}

void test_bread()
{
	struct address_space *addr;
	addr = bread(2);
//	dump(addr->data, 0, 512);
}

#define RANDOM_SIZE 1
int *random_cluster;

void make_number()
{
	
}

int main()
{
	unsigned int i = 0;
	init_all();

	make_number();

	for (; i < 100000; ++i) {
//		test_direct_read();
		test_bread();
	}
	return 0;
}
