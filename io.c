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
        fd = open("fat32.img", O_RDONLY);
        if (fd == -1) {
                perror("open");
                exit(1);
        }
}

void read8blocks(void *buf, unsigned int start_block)
{
	lseek(fd, start_block * 512, SEEK_SET);
	read(fd, buf, 4096);
}

