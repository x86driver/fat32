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

void search_dir(int fd, char *filename)
{
	struct address_space *addr = NULL;
	struct dir_entry *dir = NULL;

	fat_get_entry(&addr, &dir);
	do {
		if (dir->name[0] == 0)		/* no more files */
		  break;
		if (dir->name[0] == 0xe5)	/* deleted file */
		  continue;
		if (dir->attr == 0x0f) {	/* subcomponent long name */
			if (fat_parse_long(&addr, &dir, filename, fd) != 0) {
				printf("Parse long name failed!\n");
			}
		}

		/* regular file */
		printf("Name: %s\n", dir->name);
		unsigned int clus = (dir->starthi << 16 | dir->start);
		printf("Size: %d\n", dir->size);
		printf("Data cluster: %d\n", clus);
	} while (fat_get_entry(&addr, &dir) == 0);

//	printf("file count: %d\n", file_count);
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

int file_open(char *filename)
{
	int fd = find_empty_fd();
	search_dir(fd, filename);
	return fd;
}

int file_read(int fd)
{
	dump_file(fd);
	return 0;
}

void test_func()
{
	int fd = file_open("thisisalongname.gogo");
	file_read(fd);
//	list_all_cluster(2);
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
