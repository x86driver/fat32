#include "fat32.h"
#include "superblock.h"
#include "buffer.h"
#include "page.h"
#include "mm.h"
#include "dir.h"
#include "lib.h"

#ifdef DEBUG_MEMORY_USAGE
extern unsigned int memory_usage;
#endif

struct FAT32 fat;

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

void readdir()
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
//	int fd = file_open("TiMe.c");
//	int fd = file_open("inc1.txt");
//	int fd = file_open("8192.txt");
//	int fd = file_open("123456789.abcde");
//	int fd = file_open("Thisisa_longfile.mydata.ok");
	int fd = file_open("b.txt");

	unsigned int size_array[] = {16384, 2862};
	FILE *fp = fopen("a.dat", "wb");

	write_file(fd, mybuf, fp, size_array, sizeof(size_array)/sizeof(unsigned int));

	fclose(fp);
}

int main()
{
	init_all();
	test_func();
//	readdir();
#ifdef DEBUG_MEMORY_USAGE
	printf("\nMemory usage: %d\n", memory_usage);
#endif

	return 0;
}
