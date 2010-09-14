#include "fat32.h"
#include "dir.h"
#include "page.h"
#include "buffer.h"
#include "mm.h"
#include "lib.h"

struct msdos_sb dosb;

void get_fat_info(unsigned int fat32_sec);
unsigned char *read_sec(unsigned int sector);

static char buf_temp[4096];

static inline unsigned int find_first_partition()
{
	direct_read_sector(buf_temp, 0);
        struct Partition *partition = (struct Partition*)((char*)buf_temp+0x1be);
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
	direct_read_sector(buf_temp, fat32_sec);
        memcpy((void*)&fat, buf_temp, sizeof(fat));
}

void list_all_cluster(unsigned int first_clus)
{
	unsigned int next_clus = first_clus;
	do {
		printf("%d -> ", next_clus);
		fflush(NULL);
		next_clus = fat_next_cluster(next_clus);
	} while (next_clus != 0x0FFFFFFF);
	printf("end\n");
}

unsigned int fat_next_cluster(unsigned int currentry)
{
	struct address_space *addr = bread_sector(dosb.first_fat_sec + ((currentry * 4) / SECTOR_SIZE));
	unsigned char *ptr = (unsigned char*)addr->data;
	return *(unsigned int*)((ptr + ((currentry * 4) & (SECTOR_SIZE - 1))));
}

int __fat_get_entry_slow(struct address_space **addr, struct dir_entry **de)
{
	if (unlikely(dosb.cur_dir_clus == 0x0FFFFFFF))
		return -1;
	*addr = bread_cluster(dosb.cur_dir_clus);
	*de = (struct dir_entry*)(*addr)->data;
	dosb.cur_dir_clus = fat_next_cluster(dosb.cur_dir_clus);
	return 0;
}

int fat_get_entry(struct address_space **addr, struct dir_entry **de)
{
	static int count = 0;
	if (*addr && *de &&
		(*de - (struct dir_entry*)(*addr)->data) < (4096 / sizeof(struct dir_entry)) - 1) {
		(*de)++;
		++count;
		return 0;
	}
	return __fat_get_entry_slow(addr, de);
}

static inline int fat_cmp_name(char *filename, char *search_name)
{
	if (is_short(filename) && is_short(search_name)) {	/* short name */
		return memcmp(filename, search_name, 11);
	}

	if (strlen(filename) != strlen(search_name))
		return -1;
	return memcmp(filename, search_name, strlen(filename));
}

/* return:
   0: parse ok, but we don't need
   1: parse ok, and we need
  -1: parse failed
*/
int fat_parse_long(struct address_space **addr, struct dir_entry **de, char *search_name, int fd, int search)
{
	char filename[260];
	unsigned char slot;
	struct dir_long_entry *dle = (struct dir_long_entry*)*de;
	unsigned char checksum = dle->alias_checksum;
	if (!(dle->id & 0x40)) {
		printf("Parse error!");
		return -1;
	}

	slot = dle->id & ~0x40;
	filename[slot*13] = '\0';
	while (1) {
		--slot;
		namecpy(filename + slot * 13, dle->name0_4, 5);
		namecpy(filename + slot * 13 + 5, dle->name5_10, 6);
		namecpy(filename + slot * 13 + 11, dle->name11_12, 2);

		if (slot == 0)
			break;
		fat_get_entry(addr, de);
		dle = (struct dir_long_entry*)*de;
		if (dle->alias_checksum != checksum) {
			printf("Checksum1 error!\n");
			return -1;
		}
	}
	fat_get_entry(addr, de);
	if (fat_checksum((*de)->name) != checksum) {
		printf("long name checksum error\n");
		return -1;
	}
	if (search == 0) {
		printf("%s, %d bytes\n", filename, (*de)->size);
		return 0;
	}
	if (is_short(filename)) {
		char dstfile[12];
		memset((void*)&dstfile[0], 0x20, sizeof(dstfile));
		fmtfname(dstfile, filename);
		memcpy(filename, dstfile, 12);
	}
	if (fat_cmp_name(filename, search_name) == 0) {
		fd_pool[fd].cur_clus = fd_pool[fd].cluster = ((*de)->starthi << 16 | (*de)->start);
		fd_pool[fd].size = (*de)->size;
		printf("Found %s @ %d, %d bytes\n", filename, fd_pool[fd].cluster, (*de)->size);
		return 1;
	}
	return 0;
}
