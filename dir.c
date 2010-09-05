#include "fat32.h"

struct msdos_sb dosb;

void init_fat()
{
	unsigned int FATSz;
        fat_table = find_first_partition();
        get_fat_info(fat_table);

        if (fat.BPB_FATSz16 != 0)
                FATSz = fat.BPB_FATSz16;
        else
                FATSz = fat.BPB_FATSz32;

        dosb.root_sec = (fat_table) + ((fat.BPB_RootEntCnt*32)+(fat.BPB_BytsPerSec-1))/fat.BPB_BytsPerSec;
        dosb.first_fat_sec = fat_table + fat.BPB_ResvdSecCnt;
        dosb.first_data_sec = fat.BPB_ResvdSecCnt+(fat.BPB_NumFATs * FATSz) + RootDirSectors;
	dosb.sec_per_clus = fat.BPB_SecPerClus;
}

void get_fat_info(unsigned int fat32_sec)
{
        unsigned char *ptr = read_sec(fat32_sec);
        memcpy((void*)&fat, ptr, sizeof(fat));
        printf("%s\n", fat.BS_OEMName);
        printf("size: %d\n", fat.BPB_BytsPerSec);
}

void list_all_cluster(unsigned int first_clus)
{
	unsigned int next_clus = first_clus;
	do {
		printf("%d -> ", next_clus);
		next_clus = find_next_cluster(next_clus);
	} while (next_clus != 0x0FFFFFFF);
	printf("end\n");
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

