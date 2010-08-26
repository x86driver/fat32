#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

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

void parse_partition()
{
	struct Partition *partition = (struct Partition*)(buf+0x1be);
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
		perror("open");
		exit(1);
	}

	parse_partition();

	munmap(buf, st.st_size);
	close(fd);

	return 0;
}

