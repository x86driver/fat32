#ifndef _BUFFER_H
#define _BUFFER_H

#include "mm.h"

struct buffer_head {
	char *b_data;
};

struct bio_vec {
	unsigned int start_sector;
	unsigned int size;		/* aligned 512 bytes */
	struct page *page;		/* pointer to the page */
};

/* 會存放一個向量 */
struct bio {
	unsigned int bio_cnt;		/* count of bio_io_vec */
	unsigned int index;
	struct bio_vec *iovec;
	struct bio_vec *iovec_tail;
	int stream;
	unsigned int addr;
};

void brelse(struct buffer_head *bh);
struct buffer_head *__bread(unsigned int sector, unsigned int size);
struct buffer_head *sb_bread(unsigned int sector, unsigned int size);

inline void direct_read_sector(void *buf, unsigned int sector);
inline void direct_read(void *buf, unsigned int cluster);
#endif

