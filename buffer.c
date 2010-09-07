#include "lib.h"
#include "buffer.h"
#include "fat32.h"
#include "io.h"
#include "radix.h"
#include "superblock.h"
#include "dir.h"
#include "page.h"
#include <string.h>

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
	init_radix_allocator();
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
	dump(addr->data, 0, 512);
}

#define RANDOM_SIZE 1
int *random_cluster;

/* 測試方法
 * 限制大小為 1G 以內
 * 每次測 8192 個 cluster
 */

void test_data(int start)
{
	int i, cmp;
	void *buf = alloc_page();
	struct address_space *addr;
	for (i = start; i < start+8192; ++i) {
		direct_read(buf, i);
		addr = bread(i);
//		dump(buf, 0, 512);
//		dump(addr->data, 0, 512);
		cmp = memcmp(buf, addr->data, 4096);
		if (cmp != 0) {
			printf("\033[1;31mnot the same at %d\033[0;38m\n", i);
		} else {
			printf("Test OK at %d\n", i);
		}
	}
}

int main(int argc, char **argv)
{
	unsigned int i = 0;
	init_all();

	if (argc != 2) {
		printf("argc = %d\n", argc);
		return 0;
	}
//	for (; i < atoi(argv[1]); i+=8192) {
//		test_direct_read();
//		test_bread();
		test_data(atoi(argv[1]));
//	}
	return 0;
}
