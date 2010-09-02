#include "lib.h"
#include "buffer.h"

void brelse(struct buffer_head *bh)
{
	free(bh);
}

/* 根據相對於 partition 的 sector 讀取一個 sector (512 Bytes)
 * 目前都先忽略 size, 先假設都是 4096
 */
struct buffer_head *__bread(unsigned int sector, unsigned int size)
{
        unsigned int sector = get_sec(clus);
        unsigned char *ptr = buf + sector * SECTOR_SIZE;
	struct buffer_head *bh = (struct buffer_head*)malloc(sizeof(struct buffer_head));
	bh->b_data = ptr;
	return bh;
}

struct buffer_head *sb_bread(unsigned int sector, unsigned int size)
{
	return __bread(sector);
}
