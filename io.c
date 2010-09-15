#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "superblock.h"

static int fd;

void init_disk()
{
	fd = open("/dev/sdb", O_RDONLY);
//	fd = open("sdcard.img", O_RDONLY);
        fd = open("fat32.img", O_RDONLY);
//	fd = open("sd4g.img", O_RDONLY);
        if (fd == -1) {
                perror("open");
                exit(1);
        }
}

void read8blocks(void *buf, unsigned int start_block)
{
	if (lseek(fd, start_block * 512, SEEK_SET) == -1)
		perror("lseek");
	if (read(fd, buf, 4096) == -1)
		perror("read");
}

