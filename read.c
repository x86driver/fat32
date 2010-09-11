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
				fd_pool[fd].cur_clus = fd_pool[fd].cluster = (dir->starthi << 16 | dir->start);
				fd_pool[fd].size = dir->size;
				printf("Found %s @ %d, %d bytes with short name\n", filename, fd_pool[fd].cluster, dir->size);
				break;
			}
		}
		if (dir->attr == 0x0f) {	/* subcomponent long name */
			ret = fat_parse_long(&addr, &dir, filename, fd, search);
			if (ret == 1 && search == 1)
				break;
		} else if (search == 0) {
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

void fmtfname(char *dst, char *filename)
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

int is_short(char *filename)
{
	int len = strlen(filename);
	char *ptr = filename;
	if (len == 12) {
		if (filename[8] == '.')
			return 1;
		if (filename[9] == '.' || filename[10] == '.')
			return 0;
	} else if (len < 12) {
		while (*ptr++ != '.');
		if (ptr - filename < 10 && ((filename + len) - ptr) < 4)
			return 1;
		else
			return 0;
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

int file_read2(int fd, void *buf, unsigned int count)
{
//        unsigned int next_clus = fd_pool[fd].cluster;
        unsigned int size = 0;
	unsigned int read_count = 0;
	unsigned int remain = 0;
	struct address_space *addr;

	if (count > (fd_pool[fd].size - fd_pool[fd].pos))
		size = fd_pool[fd].size - fd_pool[fd].pos;
	else
		size = count;

	if (fd_pool[fd].pos >= fd_pool[fd].size)
		return 0;

	//判斷是否該 cluster 未讀完
	//這裡應該要判斷, 這次的 read 是否剛好讀到 4096 邊界
	//如果是, 才需要呼叫 fat_next_cluster
	//如果不是, 就要保留 cur_clus
	//同時還要判斷, 這次的 read 大小是多少, 目前都沒有判斷!!
	if ((fd_pool[fd].pos & 4095) != 0) {
		remain = ((fd_pool[fd].pos / 4096) + 1) * 4096;
		remain -= fd_pool[fd].pos;
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data + fd_pool[fd].pos, remain);
		buf += remain;
//		fd_pool[fd].cur_clus = fat_next_cluster(fd_pool[fd].cur_clus);
		fd_pool[fd].pos += remain;
		size -= remain;
		read_count += remain;
		if (size == 0)
			return read_count;
	}

        if (size < 4096) {
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data, size);
		read_count += size;
		fd_pool[fd].pos += read_count;
                return read_count;
        }

        do {
                direct_read(buf, fd_pool[fd].cur_clus);
                size -= 4096;
		buf += 4096;
		read_count += 4096;
                fd_pool[fd].cur_clus = fat_next_cluster(fd_pool[fd].cur_clus);
        } while (size >= 4096 && fd_pool[fd].cur_clus != 0x0FFFFFFF);

	if (size > 0) {
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data, size);
		read_count += size;
	}
	fd_pool[fd].pos = read_count;
	return read_count;
}

int _file_align_read(int fd, void *buf, unsigned int count)
{
	struct address_space *addr;
	unsigned int read_count = 0;

	if (fd_pool[fd].pos == fd_pool[fd].size)
		return 0;

	if ((count + fd_pool[fd].pos) > fd_pool[fd].size) {
		count = fd_pool[fd].size - fd_pool[fd].pos;
	}

	if (count < 4096) {
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data, count);
		fd_pool[fd].pos += count;
		return count;
	} else if (count == 4096) {
			direct_read(buf, fd_pool[fd].cur_clus);
			fd_pool[fd].pos += 4096;
			fd_pool[fd].cur_clus = fat_next_cluster(fd_pool[fd].cur_clus);
			return 4096;
	} else { // > 4096
		do {
			direct_read(buf, fd_pool[fd].cur_clus);
			count -= 4096;
			read_count += 4096;
			buf += 4096;
			fd_pool[fd].cur_clus = fat_next_cluster(fd_pool[fd].cur_clus);
		} while (count >= 4096 && fd_pool[fd].cur_clus != 0x0FFFFFFF);
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data, count);
		read_count += count;
		fd_pool[fd].pos += read_count;
		return read_count;
	}
}

int _file_normal_read(int fd, void *buf, unsigned int count)
{
	struct address_space *addr;
	unsigned int distance, remain, file_len;

	if (fd_pool[fd].pos == fd_pool[fd].size)
		return 0;

	distance = (fd_pool[fd].pos / 4096) * 4096;
	distance = fd_pool[fd].pos - distance;
	remain = ((fd_pool[fd].pos / 4096) + 1) * 4096;
	remain -= fd_pool[fd].pos;
	file_len = fd_pool[fd].size - fd_pool[fd].pos;

	count = (count > file_len ? file_len : count);

	if (count <= remain) {
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data + distance, count);
		fd_pool[fd].pos += count;
		if (count == remain)
			fd_pool[fd].cur_clus = fat_next_cluster(fd_pool[fd].cur_clus);
		return count;
	} else {
		addr = bread_cluster(fd_pool[fd].cur_clus);
		memcpy(buf, addr->data + distance, remain);
		buf += remain;
		count -= remain;
		fd_pool[fd].pos += remain;
		fd_pool[fd].cur_clus = fat_next_cluster(fd_pool[fd].cur_clus);
		return remain + _file_align_read(fd, buf, count);
	}

}

int file_read(int fd, void *buf, unsigned int count)
{
	if (fd_pool[fd].pos == 0 || (fd_pool[fd].pos & 4095) == 0) {
		return _file_align_read(fd, buf, count);
	} else {
		return _file_normal_read(fd, buf, count);
	}
}

void write_file(int fd, char *mybuf, FILE *fp, unsigned int *size, int count)
{
	int ret, i = 0;
	for (; i < count; ++i) {
        	ret = file_read(fd, mybuf, *size++);
	        fwrite(mybuf, ret, 1, fp);
	}
}

void test_func()
{
	char *mybuf = malloc(65536);
	if (mybuf == NULL)
		perror("malloc");
//	int fd = file_open("thisisalongname.gogo");
//	int fd = file_open("a.txta");
//	int fd = file_open("a.txt");
//	int fd = file_open("acct.C");
	int fd = file_open("TiMe.c");
//	int fd = file_open("inc1.txt");
//	int fd = file_open("8192.txt");

	unsigned int size_array[] = {16384, 2862};
	FILE *fp = fopen("a.dat", "wb");

	write_file(fd, mybuf, fp, size_array, sizeof(size_array)/sizeof(unsigned int));

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
