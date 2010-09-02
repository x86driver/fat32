#include "lib.h"
#include "buffer.h"

extern unsigned char *buf;

/* bread 流程:
 * 流程應該是當上層呼叫 bread 後, 會開始建構 buffer_head
 * 之後呼叫 submit_bh() 把 buffer_head 傳到 bio 層
 * 然後再次呼叫 submit_bio() 直接讀資料
 */

/* 新函式 -- struct buffer_head *async_read(int sector)
 * AP 流程應該可以這樣做:
 * 每次要讀取就呼叫 async_read(), 但該函式不做真正的讀取
 * 當確定好所有要讀取的 block 後, 再呼叫 submit_bio()
 * 優點是我們讀 FAT table 時可以用 bread, 但此時先不讀資料部份
 * 等到 FAT table 全部 parse 完後, 在呼叫 submit_bio()
 */

/* bio_vec 到底要放哪裡？
 * 他應該要是一個陣列,  或者是 linked-list
 * 但為了效能考量, 我們在初始化的時候先分配一個大陣列
 * 之後每個 bio 限制裡面的 cnt 是陣列的數量
 */

/* 有關 bio 以及問題
 * 1. 每次讀取的最小單位是 8 block, 1 page, 4096 bytes
 * 2. 每個 bio_vec 只能對應一個 page, 4096 bytes
 * 問題:
 * 1. 要把資料讀到哪裡? 資料位置要設定為連續, 或者根據 bio_vec?
 * 2. 是否真的需要根據 bio_vec? 因為目前看來沒有什麼情況需要
 *    把資料讀到不同地方 (在 multithread 下才有機會)
 * 3. 那是否可以仍舊根據 bio_vec, 但又能在不影響效能下,
 *    把資料讀到連續的地方?
 * 4. 在 struct bio 新增一個欄位 stream, 當 stream = 1,
 *    代表資料是連續讀到 bio->addr
 * 5. 此時每做完一次 submit_bio() 就必須更新整個 page
 */

/* request_queue
 * 當我們要合併 block 時會用到的資料結構
 * 一個 request_queue 可以有多個 bio, 此時會在這裡做合併
 */

/* __find_get_block() 流程:
 * 參數是 block
 * 1. 利用 struct *page = find_get_page() 搜尋是否在 radix tree 中
 * 2. 開始根據 page->bh 找出 bh 這個串列所有的 bh,
 *    然後判斷 bh->block 是否就是傳進來的參數 block
 * 3. 找到後, 傳回該 buffer_head
 * 4. 完成!
 */

/* alloc_page_buffers(struct page*,  int size) 流程:
 * 會根據 size 分配 n 個 buffer_head (n 應該 = 4096/size)
 */

/* grow_buffers(int block) 流程:
 * 1. 呼叫 find_or_create_page() 去快取區中新增一個新的頁面
 * 2. 若該頁面是被新增的, 呼叫 alloc_page_buffers()
 * 3. 呼叫 init_page_buffers() 初始化『所有』的 buffer_head
 */

/* __getblk() 流程:
 * 1. 呼叫 __find_get_block() 檢查是否在快取區中
 * 2. 如果沒有, 就呼叫 grow_buffers() 分配一個新的
 * 3. 傳回分配到的 buffer_head
 * 4. 完成!
 */

/* merge_request(struct bio*)
 * 1. 去 request_queue 找第一個 request 看是否能跟 bio 合併
 * 2. 不行的話找最後一個 request
 * 3. 大致算完成
 */

/* start_io()
 * 開始處理 request_queue 所有的 request, 把他們讀進來
 */

/* bread_direct()
 * 1. 呼叫 __bread()
 * 2. 呼叫 start_io()
 */

/* submit_bio(struct bio*)
 * 1. 調整 bio->bi_sector 把相對 sector 根據 partition 轉成絕對 sector
 * 2. 呼叫 merge_request 合併 bio 利用 request_queue 找出是否有可以合併的 sector
 * 3. 完成!
 */

/* submit_bh(struct buffer_head*) 流程:
 * 1. 分配一個新的 bio
 * 2. bio->size = bh->size
 * 3. 設定 bio_vec[0] 設定為 bh 的各項參數
 * 4. bio->bio_cnt = 1, bio->index = 0; (目前準備要處理的 index)
 * 5. 呼叫 submit_bio()
 * 6. 
 */

/* __bread() 新流程:
 * 1. 參數可以是好幾個 block, 但單位是 block, 回傳值是 buffer_head*
 * 2. 呼叫 bh = __getblk(), 以便在頁面快取區中找到資料
 * 3. 注意, 傳回的 bh 可能會有好幾個
 * 4. 呼叫 submit_bh() 把該 bh 傳進去
 * 5. 
 */
void submit_bio(struct bio *bio)
{
}

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
