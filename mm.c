#include "page.h"
#include "mm.h"
#include "radix.h"
#include "lib.h"

struct address_space *address_space_array;
unsigned int address_space_index;
struct radix_tree *radix_buffer[RADIX_THRESHOLD];
unsigned int radix_buffer_index;
int radix_buffer_array_index;
unsigned int radix_buffer_cur_max;

void init_address_space()
{
	//數量是128*128 = 16384
        address_space_array = (struct address_space*)page_malloc(CACHE_ENTRY * sizeof(struct address_space));
        address_space_index = 0;
}

void init_radix_allocator()
{
	radix_buffer[0] =
		any_malloc(RADIX_SIZE_LV2 * sizeof(struct radix_tree) / RADIX_THRESHOLD);
	radix_buffer_index = 0;
	radix_buffer_array_index = 0;
	radix_buffer_cur_max = RADIX_SIZE_LV2 / RADIX_THRESHOLD;
}

struct radix_tree *alloc_radix_tree()
{
	if (unlikely(radix_buffer_index >= radix_buffer_cur_max)) {
		BUG_ON(radix_buffer_array_index >= RADIX_THRESHOLD - 1);
		radix_buffer[++radix_buffer_array_index] =
			any_malloc(RADIX_SIZE_LV2 * sizeof(struct radix_tree) / RADIX_THRESHOLD);
		radix_buffer_cur_max = RADIX_SIZE_LV2 / RADIX_THRESHOLD;
		radix_buffer_index = 0;
	}
	return radix_buffer[radix_buffer_array_index] + radix_buffer_index++;
}

#if 0
int main()
{
	init_radix_allocator();
	int i;
	struct radix_tree *ra[4096];
	for (i = 0; i < 4096; ++i) {
		ra[i] = alloc_radix_tree();
		ra[i]->next[0] = i;
	}
	for (i = 0; i < 4096; ++i) {
		printf("%d\n", ra[i]->next[0]);
	}
	return 0;
}
#endif
