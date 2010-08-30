#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fat32.h"

#define SECTOR_SIZE 512

unsigned char *cache;
unsigned char *cache512;
unsigned char *buf;

struct Partition {
	unsigned char status;
	unsigned char head;
	unsigned char sector;
	unsigned char cylinder;
	unsigned char type;
	unsigned char endhead;
	unsigned char endsector;
	unsigned char endcylinder;
	unsigned int startlba;
	unsigned int totalsec;
};

static inline unsigned int find_first_partition()
{
	struct Partition *partition = (struct Partition*)(buf+0x1be);
	return partition->startlba;
#if 0
	unsigned int i = 0;
	for (; i < 4; ++i) {
		printf("Partition #%d\n", i);
		printf("status: 0x%x\n", partition->status);
		printf("Start head: %d, sector: %d, cylinder: %d\n",
			partition->head, partition->sector, partition->cylinder);
		printf("type: 0x%x\n", partition->type);
		printf("End head: %d, sector: %d, cylinder: %d\n",
			partition->endhead, partition->endsector, partition->endcylinder);
		printf("LBA Start: %d, total: %d, size: %d (%d)\n\n",
			partition->startlba, partition->totalsec,
			partition->totalsec*SECTOR_SIZE, (partition->totalsec*SECTOR_SIZE)/1048576);
		printf("===============================================\n\n");
		partition += sizeof(struct Partition);
	}
#endif
}

struct FAT32 fat;
unsigned int FATSz;
unsigned int fat_table;
unsigned int RootDirSectors;
unsigned int FirstFatSector;
unsigned int FirstDataSector;

static inline unsigned int get_sec(unsigned int cluster)
{
        return ((cluster - 2) * fat.BPB_SecPerClus) + FirstDataSector;
}

unsigned int find_next_cluster(unsigned int currentry)
{
	unsigned int cluster = *(unsigned int*)(buf + (FirstFatSector * SECTOR_SIZE + currentry * 4));
	return cluster;
}

/* Usage:
 * buf = read_sec(xxx);
 * Beware size of buf is 512
 */
unsigned char *read_sec(unsigned int sector)
{
	memset(cache, 0, 4096);
	unsigned char *ptr = buf + sector * SECTOR_SIZE;
	memcpy(cache512, ptr, 512);
	return cache512;
}

/* Usage:
 * buf = read_clus(xxx);
 */
unsigned char *read_clus(unsigned int clus)
{
        unsigned int sector = get_sec(clus);
        unsigned char *ptr = buf + sector * SECTOR_SIZE;
        memcpy(cache, ptr, 4096);
	return cache;
}

void get_fat_info(unsigned int fat32_sec)
{
//      fat = (struct FAT32*)(buf+(fat32_sec*SECTOR_SIZE));
        unsigned char *ptr = read_sec(fat32_sec);
        memcpy((void*)&fat, ptr, sizeof(fat));
        printf("%s\n", fat.BS_OEMName);
        printf("size: %d\n", fat.BPB_BytsPerSec);
}

void init_fat()
{
        fat_table = find_first_partition();
        get_fat_info(fat_table);

        if (fat.BPB_FATSz16 != 0)
                FATSz = fat.BPB_FATSz16;
        else
                FATSz = fat.BPB_FATSz32;

        RootDirSectors = (fat_table) + ((fat.BPB_RootEntCnt*32)+(fat.BPB_BytsPerSec-1))/fat.BPB_BytsPerSec;
        FirstFatSector = fat_table + fat.BPB_ResvdSecCnt;
        FirstDataSector = fat.BPB_ResvdSecCnt+(fat.BPB_NumFATs * FATSz) + RootDirSectors;
}

void read_content(unsigned int clus)
{
	unsigned int sector = get_sec(clus);
	unsigned char *text = buf+sector*SECTOR_SIZE;
	printf("%s", text);
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

void dump_file(unsigned int first_clus, unsigned int size)
{
	FILE *fp = fopen("e.dat", "wb");
	unsigned int next_clus = first_clus;
	unsigned char *ptr;
        if (size <= 4096) {
		ptr = buf + get_sec(next_clus) * SECTOR_SIZE;
                fwrite(ptr, size, 1, fp);
		fclose(fp);
		return;
	}

	do {
		ptr = buf + get_sec(next_clus) * SECTOR_SIZE;
		fwrite(ptr, 4096, 1, fp);
		size -= 4096;
		next_clus = find_next_cluster(next_clus);
	} while (size >=4096 && next_clus != 0x0FFFFFFF);

	ptr = buf + get_sec(next_clus) * SECTOR_SIZE;
	fwrite(ptr, size, 1, fp);
	fclose(fp);
}

void fmtfname(char *dst, const char *src)
{
//format file name to 8 3 (DOS)
//WARNING: make sure 'dst' must allocated 11 bytes
// and filled with 0x20 (space)

	int count = 0;
	while (1) {
        	if (*src == '.') {
        		dst += 8-count;
	        	++src;
        		continue;
        	} else if (*src == '\0')
        		break;

	        *dst++ = *src++;
        	++count;
	}
}

void read_file()
{
//	unsigned int datasec = get_sec(2);
//	struct dir_entry *dir = (struct dir_entry*)(buf+(datasec*SECTOR_SIZE));
	struct dir_entry *dir = (struct dir_entry*)read_clus(2);
	unsigned int i = 0;
	unsigned int long_flag = 0;

	for (; i < 16; ++i) {
		if (dir->DIR_Name[0] == 0)	/* no more files */
			break;
		if (dir->DIR_Name[0] == 0xe5) {	/* deleted file */
			++dir;
			continue;
		}
		if (dir->DIR_Attr == 0x0f) {	/* subcomponent long name */
			long_flag = 1;
			++dir;
			continue;
		} else {
			if (long_flag == 0)
				printf("Filename: %s\n", dir->DIR_Name);
			else {
				printf("Filename: %s (It has long name)\n", dir->DIR_Name);
				long_flag = 0;
			}
		}

		unsigned int clus = (dir->DIR_FstClusHI << 16 | dir->DIR_FstClusLO);
		printf("Size: %d\n", dir->DIR_FileSize);
		printf("Data cluster: %d\n", clus);
//		if (dir->DIR_Name[0] == 'E')
//			dump_file(clus, dir->DIR_FileSize);
		++dir;
	}
}

int main()
{
	int fd = open("fat32.img", O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	struct stat st;
	fstat(fd, &st);

	cache = malloc(4096);
	cache512 = malloc(512);

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ((char*)buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	init_fat();
	read_file();

	free(cache);
	free(cache512);
	munmap(buf, st.st_size);
	close(fd);

	return 0;
}

