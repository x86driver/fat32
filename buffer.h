struct buffer_head {
	char *b_data;
};

void brelse(struct buffer_head *bh);
struct buffer_head *__bread(unsigned int sector, unsigned int size);
struct buffer_head *sb_bread(unsigned int sector, unsigned int size);
