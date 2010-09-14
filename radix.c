#include "radix.h"
#include "mm.h"
#include "lib.h"
#include "page.h"

struct radix_tree *radix;
struct radix_tree *radix_buffer;

struct address_space *find_or_create(struct radix_tree * restrict radix,
		unsigned int cluster, int * restrict create)
{
	struct radix_tree *lv1_node, *lv2_node, *lv3_node;
	lv1_node = radix->next[cluster >> 12];
	lv2_node = lv1_node->next[(cluster >> 6) & 0x03f];
	if (lv2_node != NULL) {
		lv3_node = lv2_node->next[cluster & 0x03f];
		if (lv3_node != NULL) {
			*create = FIND_NODE;
			return (struct address_space*)lv3_node;
		} else {
			struct address_space *addr = alloc_address_space();
			BUG_ON(addr == NULL);
			addr->cluster = cluster;
			*create = NEW_NODE;
			lv2_node->next[cluster & 0x03f] = (struct radix_tree*)addr;
			return addr;
		}
	} else {
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
