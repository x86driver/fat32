#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "fat32.h"

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

/*
void read_sec(unsigned int lba)
{
	lseek(fd, lba*512, SEEK_SET);
	read(fd, (void*)&sector[0], 512);
}
*/

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
			partition->totalsec*512, (partition->totalsec*512)/1048576);
		printf("===============================================\n\n");
		partition += sizeof(struct Partition);
	}
#endif
}

struct FAT32 *fat;
unsigned int FATSz;
unsigned int fat_table;
unsigned int RootDirSectors;
unsigned int FirstDataSector;

void get_fat_info(unsigned int fat32_sec)
{
	fat = (struct FAT32*)(buf+(fat32_sec*512));
	printf("%s\n", fat->BS_OEMName);
	printf("size: %d\n", fat->BPB_BytsPerSec);
}

void init_fat()
{
        fat_table = find_first_partition();
        get_fat_info(fat_table);

        if (fat->BPB_FATSz16 != 0)
                FATSz = fat->BPB_FATSz16;
        else
                FATSz = fat->BPB_FATSz32;

        RootDirSectors = (fat_table) + ((fat->BPB_RootEntCnt*32)+(fat->BPB_BytsPerSec-1))/fat->BPB_BytsPerSec;
        FirstDataSector = fat->BPB_ResvdSecCnt+(fat->BPB_NumFATs * FATSz) + RootDirSectors;
}

static inline unsigned int get_sec(unsigned int cluster)
{
	return ((cluster - 2) * fat->BPB_SecPerClus) + FirstDataSector;
}

void read_content(unsigned int clus)
{
	unsigned int sector = get_sec(clus);
	unsigned char *text = buf+sector*512;
	printf("%s", text);
}

void read_file()
{
	unsigned int datasec = get_sec(2);
	struct dir_entry *dir = (struct dir_entry*)(buf+(datasec*512));
	unsigned int i = 0;

	for (; i < 16; ++i) {
		if (dir->DIR_Name[0] == 0)
			break;
		printf("Filename: %s\n", dir->DIR_Name);
		unsigned int clus = (dir->DIR_FstClusHI << 16 | dir->DIR_FstClusLO);
		printf("Size: %d\n", dir->DIR_FileSize);
		read_content(clus);
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

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ((char*)buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	init_fat();
	read_file();

	munmap(buf, st.st_size);
	close(fd);

	return 0;
}

