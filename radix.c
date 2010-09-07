#include <stdio.h>
#include <string.h>
#include "radix.h"
#include "mm.h"
#include "lib.h"
#include "page.h"

struct radix_tree *radix;
struct radix_tree *radix_buffer;	/* 用來給第二層使用的, 第二層會有 4096 個 */

/* 問題
 * 一般讀資料時 我們都不用把他存在 buffer (用 direct_read)
 * 讀 FAT table 時我們就用 cache 的方式
 * 而 radix tree 不用考慮一次搜尋多個 page
 * 因為讀 FAT table 一次也只能處理一個 cluster
 */

/* find_or_create 找不到就新增
 * 輸入: radix tree 以及要找的資料
 * 所謂要找的資料指的是 cluster
 * 傳回 address_space (描述實際要從哪裡拿到這筆資料)
 * 注意:
 * 1. 此函式永遠不會失敗, 因為就算找不到也會新增
 * 2. 無論如何一定要找2次, 才能找到最後一個 node
 * 3. 最後一個 node->next 可以轉型成為 address_space
 * 4. 第一層我們預設已經分配好空間, 所以第一層一定可以找到東西
 * 5. create 的用途是通知上面這個 node 是新增的, 所以
 *    應該要放東西進來 node->data, 若該點不是新增的,
 *    就可以直接從 node->data 讀到東西
 */

struct address_space *find_or_create(struct radix_tree * restrict radix,
		unsigned int cluster, int * restrict create)
{
	struct radix_tree *lv1_node, *lv2_node, *lv3_node;
	lv1_node = radix->next[cluster >> 12];
//	lv2_node = lv1_node->next[cluster & 0x07f];
	lv2_node = lv1_node->next[(cluster >> 6) & 0x03f];
	if (lv2_node != NULL) {
		lv3_node = lv2_node->next[cluster & 0x03f];
		if (lv3_node != NULL) {
			*create = FIND_NODE;
			return (struct address_space*)lv3_node;
		} else { //第一層找到, 第二層找到, 第三層沒找到
			struct address_space *addr = alloc_address_space();
			BUG_ON(addr == NULL);
			addr->cluster = cluster;
			*create = NEW_NODE;
			lv2_node->next[cluster & 0x03f] = (struct radix_tree*)addr;
			return addr;
		}
	} else { //第一層找到, 第二層沒找到
		//直接新增兩層
		lv2_node = lv1_node->next[(cluster >> 6) & 0x03f] = alloc_radix_tree();
		BUG_ON(lv2_node == NULL);
		lv3_node = lv2_node->next[cluster & 0x03f];
		struct address_space *addr = alloc_address_space();
		BUG_ON(addr == NULL);
		addr->cluster = cluster;
		*create = NEW_NODE;
		lv2_node->next[cluster & 0x03f] = (struct radix_tree*)addr;
		return addr;
	}
}

void init_radix_tree()
{
	int i;
	radix = (struct radix_tree*)page_malloc(sizeof(struct radix_tree));
	memset(radix, 0, sizeof(struct radix_tree));
	void *buf = page_malloc(sizeof(struct radix_tree) * RADIX_SIZE);
	memset(buf, 0, sizeof(struct radix_tree) * RADIX_SIZE);
	for (i = 0; i < RADIX_SIZE; ++i) {
		radix->next[i] = (struct radix_tree*)buf + i;
	}
}

void lookup2(unsigned int cluster)
{
	struct address_space *addr;
	int create = 0;
	char buf[64];
//	addr = find_or_create(radix, cluster, &create);
	addr = lookup(cluster, &create);
        if (create == FIND_NODE) {
                printf("Cluster %d: %s\n", cluster, (char*)addr->data);
        } else {
		printf("New node(%d), add something!\n", cluster);
		snprintf(buf, sizeof(buf), "Hello world, this cluster is pointer to --> %d", cluster);
		addr->data = alloc_page();
		BUG_ON(addr->data == NULL);
                strcpy((char*)addr->data, buf);
        }
}

#if 0
int main()
{
	init_address_space();
	init_radix_allocator();
	init_radix_tree();
	int i;
#define START 0
	for (i = START; i < START + CACHE_ENTRY; ++i) {
		lookup2(i);
	}
	for (i = START; i < START + CACHE_ENTRY; ++i) {
		lookup2(i);
	}
	return 0;
}
#endif
