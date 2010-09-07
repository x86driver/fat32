#include "fat32.h"
#include "dir.h"
#include "page.h"
#include "buffer.h"
#include "mm.h"
#include <string.h>
#include <stdio.h>

struct msdos_sb dosb;

void get_fat_info(unsigned int fat32_sec);
unsigned char *read_sec(unsigned int sector);

//改成用 direct_read
//因為不需要 buffer 起來
static inline unsigned int find_first_partition()
{
	void *buf = alloc_page();
	direct_read_sector(buf, 0);
        struct Partition *partition = (struct Partition*)((char*)buf+0x1be);
        return partition->startlba;
}

void init_fat()
{
	unsigned int FATSz;
        unsigned int fat_table = find_first_partition();
        get_fat_info(fat_table);

        if (fat.BPB_FATSz16 != 0)
                FATSz = fat.BPB_FATSz16;
        else
                FATSz = fat.BPB_FATSz32;

        dosb.root_sec = (fat_table) + ((fat.BPB_RootEntCnt*32)+(fat.BPB_BytsPerSec-1))/fat.BPB_BytsPerSec;
        dosb.first_fat_sec = fat_table + fat.BPB_ResvdSecCnt;
        dosb.first_data_sec = fat.BPB_ResvdSecCnt+(fat.BPB_NumFATs * FATSz) + dosb.root_sec;
	dosb.sec_per_clus = fat.BPB_SecPerClus;
	dosb.cur_dir_clus = 2;
}

void get_fat_info(unsigned int fat32_sec)
{
	void *buf = alloc_page();
	direct_read_sector(buf, fat32_sec);
        memcpy((void*)&fat, buf, sizeof(fat));
        printf("%s\n", fat.BS_OEMName);
        printf("size: %d\n", fat.BPB_BytsPerSec);
}

void list_all_cluster(unsigned int first_clus)
{
	unsigned int next_clus = first_clus;
	do {
		printf("%d -> ", next_clus);
		next_clus = fat_next_cluster(next_clus);
	} while (next_clus != 0x0FFFFFFF);
	printf("end\n");
}

unsigned int fat_next_cluster(unsigned int currentry)
{
        /* 流程:
         * 1. 取得 currentry 所在的 cluster
         * 2. 找到下一個 entry
         * ☯注意： 這裡不論如何只需要讀取一次 cluster,
         *         不需要讀取下一個 cluster, 因為我們只是把找到的
         *         cluster 回傳就好
         * 『注意』： 目前尚未完成這個函式, 因為要用 bread 去讀
         */
	struct address_space *addr = bread_sector(dosb.first_fat_sec + ((currentry * 4) / SECTOR_SIZE));
	unsigned char *ptr = (unsigned char*)addr->data;
	return *(unsigned int*)((ptr + ((currentry * 4) & (SECTOR_SIZE - 1))));
//        unsigned int cluster = *(unsigned int*)(buf + (dosb.first_fat_sec * SECTOR_SIZE + currentry * 4));
//        return cluster;
}

//注意, 以下全部先註解
#if 0

/* 傳回指向 cluster 的 pointer
 * 要做的事情:
 * 1. 算出"相對於"該 partition 的 sector
 * 2. 呼叫 sb_bread
 * 3. 傳回 bh->b_data
 */
void *fat_read_clus(unsigned int cluster)
{
}

unsigned char *entry_buf = NULL;
int __fat_get_entry_slow(struct buffer_head **bh, struct inode **inod, struct dir_entry **de)
{
	bh = sb_bread();
	entry_buf = fat_read_clus(2);
	*de = (struct dir_entry*)entry_buf;
}

int fat_get_entry(struct buffer_head **bh, struct dir_entry **de)
{
	if (*bh && *de && (*de - bh->b_data < 4096)) {
		(*de)++;
		return 0;
	}
	return __fat_get_entry_slow(bh, de);
}

int fat_parse_long(struct dir_entry **de)
{
	char filename[260];
	unsigned char slot;
	struct dir_long_entry *dle = (struct dir_long_entry*)*de;
	if (!(dle->id & 0x40)) {
		printf("Parse error!");
		return -1;
	}

	slot = dle->id & ~0x40;
	/* 開始 parse 直到 id = 1 */
	while (1) {
		--slot;
		namecpy(filename + slot * 13, dle->name0_4, 5);
		namecpy(filename + slot * 13 + 5, dle->name5_10, 6);
		namecpy(filename + slot * 13 + 11, dle->name11_12, 2);

		if (slot == 0)
			break;
		fat_get_entry(de);
		dle = (struct dir_long_entry*)*de;
	}
	printf("Found %s\n", filename);
	return 0;
}

int vfat_find(char *fname)
{
	struct dir_entry *de = NULL;

	while (1) {
		fat_get_entry(&de);
		if (de->name[0] == 0xe5)
			continue;
		if (de->name[0] == 0x0)
			break;
		if (de->attr == 0x0f) {
			fat_parse_long(&de);
		}
	}
}

#endif
