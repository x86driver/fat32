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

#ifdef DEBUG_MEMORY_USAGE
unsigned int memory_usage = 0;
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

void dump_file(unsigned int first_clus, unsigned int size)
{
	FILE *fp = fopen("e.dat", "wb");
	unsigned int next_clus = first_clus;
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

void show_dir()
{
	unsigned int next_clus = 2;
	struct address_space *addr;
	int i = 0;
	int long_flag = 0;
	int file_count = 0;

	do {
		addr = bread_cluster(next_clus);
		struct dir_entry *dir = (struct dir_entry*)addr->data;
		for (i = 0; i < 4096/sizeof(struct dir_entry); ++i) {
			if (dir->name[0] == 0)		/* no more files */
			  break;
			if (dir->name[0] == 0xe5) {	/* deleted file */
				++dir;
				continue;
			}
			if (dir->attr == 0x0f) {	/* subcomponent long name */
				long_flag = 1;
				++dir;
				continue;
			} else {
				++file_count;
				if (long_flag == 0)
				  printf("Filename: %s\n", dir->name);
				else {
					printf("Filename: %s (It has long name)\n", dir->name);
					long_flag = 0;
				}
			}

			unsigned int clus = (dir->starthi << 16 | dir->start);
			printf("Size: %d\n", dir->size);
			printf("Data cluster: %d\n", clus);
			if (dir->name[0] == 'G')
			  dump_file(clus, dir->size);
			++dir;
		}
		next_clus = fat_next_cluster(next_clus);
	} while (next_clus != 0x0FFFFFFF);

	printf("file count: %d\n", file_count);
}

void test_func()
{
	show_dir();
//	list_all_cluster(2);
}

int main()
{
#ifdef DEBUG_MEMORY_USAGE
	memory_usage = 0;
#endif
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
