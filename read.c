#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "fat32.h"
#include "superblock.h"
#include "buffer.h"
#include "page.h"
#include "mm.h"
#include "dir.h"

#ifdef DEBUG_MEMORY_USAGE
extern unsigned int memory_usage;
#endif

struct FAT32 fat;

#if 0
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
        unsigned int sector = fat_get_sec(clus);
        unsigned char *ptr = buf + sector * SECTOR_SIZE;
        memcpy(cache, ptr, 4096);
	return cache;
}

void read_content(unsigned int clus)
{
	unsigned int sector = fat_get_sec(clus);
	unsigned char *text = buf+sector*SECTOR_SIZE;
	printf("%s", text);
}
#endif

//void dump_file(unsigned int first_clus, unsigned int size)
void dump_file(int fd)
{
	FILE *fp = fopen("e.dat", "wb");
	unsigned int next_clus = fd_pool[fd].cluster;
	unsigned int size = fd_pool[fd].size;
	unsigned char *ptr = alloc_page();

        if (size <= 4096) {
		direct_read(ptr, next_clus);
                fwrite(ptr, size, 1, fp);
		fclose(fp);
		return;
	}

	do {
		direct_read(ptr, next_clus);
		fwrite(ptr, 4096, 1, fp);
		size -= 4096;
		next_clus = fat_next_cluster(next_clus);
	} while (size >=4096 && next_clus != 0x0FFFFFFF);

	direct_read(ptr, next_clus);
	fwrite(ptr, size, 1, fp);
	fclose(fp);
}

void search_dir(int fd, char *filename, int search)
{
	struct address_space *addr = NULL;
	struct dir_entry *dir = NULL;
	int ret;

	fat_get_entry(&addr, &dir);
	do {
		if (dir->name[0] == 0)		/* no more files */
		  break;
		if (dir->name[0] == 0xe5)	/* deleted file */
		  continue;
		if (search == 1) {
			if (memcmp(filename, dir->name, 11) == 0) {
				fd_pool[fd].cluster = (dir->starthi << 16 | dir->start);
				fd_pool[fd].size = dir->size;
				printf("Found %s @ %d, %d bytes with short name\n", filename, fd_pool[fd].cluster, dir->size);
				break;
			}
		}
		if (dir->attr == 0x0f) {	/* subcomponent long name */
			ret = fat_parse_long(&addr, &dir, filename, fd, search);
			if (ret == 1 && search == 1)
				break;
		} else {
			/* regular file */
			printf("%s    %d bytes\n", dir->name, dir->size);
		}
	} while (fat_get_entry(&addr, &dir) == 0);
	dosb.cur_dir_clus = 2;
}

int find_empty_fd()
{
	int i = 0;
	for (; i < MAX_FD; ++i) {
		if (fd_pool[i].cluster == 0) {
			return i;
		}
	}
	printf("fd pool is full!\n");
	return -1;
}

static void fmtfname(char *dst, char *filename)
{
        //format file name to 8 3 (DOS)
        //WARNING: make sure 'dst' must allocated 11 bytes
        // and filled with 0x20 (space)
	char *src = filename;
	char *dstbuf = dst;

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
	*dst = '\0';
	file2upper(dstbuf);
}

inline void readdir()
{
	search_dir(0, NULL, 0);
}

static inline int is_short(char *filename)
{
	int len = strlen(filename);
	char *ptr = filename;
	if (len < 13) {
		//還要判斷是否是長檔名
		//比方說 a.update
		while (*ptr++ != '.');
		if (ptr - filename < 4)
			return 0;
		else
			return 1;
	}
	return 0;
}

int file_open(char *filename)
{
	char dstfile[12];
	int fd = find_empty_fd();
	if (is_short(filename) == 1) { /* 12345678.123 */
		memset((void*)&dstfile[0], 0x20, 11);
		fmtfname(dstfile, filename);
		search_dir(fd, dstfile, 1);
	} else {
		search_dir(fd, filename, 1);
	}
	return fd;
}

int file_read(int fd, void *buf, unsigned int count)
{
        unsigned int next_clus = fd_pool[fd].cluster;
        unsigned int size = (count > fd_pool[fd].size ? fd_pool[fd].size : count);

	if (fd_pool[fd].pos < count)
		fd_pool[fd].pos = size;
	else
		return 0;

        if (size <= 4096) {
                direct_read(buf, next_clus);
                return fd_pool[fd].pos;
        }

        do {
                direct_read(buf, next_clus);
                count -= 4096;
		buf += 4096;
                next_clus = fat_next_cluster(next_clus);
        } while (count >= 4096 && next_clus != 0x0FFFFFFF);

        direct_read(buf, next_clus);
	return fd_pool[fd].pos;
}

void test_func()
{
	char *mybuf = malloc(19245);
	if (mybuf == NULL) {
		perror("malloc");
	}
	int fd = file_open("thisisalongname.gogo");
//	int fd = file_open("a.txta");
//	int fd = file_open("TiMe.C");
	int ret = file_read(fd, mybuf, 19245);
	FILE *fp = fopen("a.dat", "wb");
	fwrite(mybuf, ret, 1, fp);
	fclose(fp);
}

int main()
{
	init_all();
	test_func();
#ifdef DEBUG_MEMORY_USAGE
	printf("\nMemory usage: %d\n", memory_usage);
#endif

	return 0;
}

#if 0
int main()
{
	int fd = open("fat32.img", O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	struct stat st;
	fstat(fd, &st);

	sb = (struct super_block*)malloc(sizeof(struct super_block));
	init_superblock();

	cache = malloc(4096);
	cache512 = malloc(512);

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if ((char*)buf == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	init_fat();
	test_func();

	free(cache);
	free(cache512);
	free(sb);

	munmap(buf, st.st_size);
	close(fd);

	return 0;
}

#endif
