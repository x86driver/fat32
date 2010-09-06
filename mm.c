#include "page.h"
#include "mm.h"
#include "radix.h"
#include "lib.h"

struct address_space *address_space_array;
unsigned int address_space_index;
struct radix_tree *radix_buffer;
unsigned int radix_buffer_index;
unsigned int radix_buffer_cur_max;

#define RADIX_SIZE_LV2 RADIX_SIZE * RADIX_SIZE
#define RADIX_THRESHOLD 4

void init_address_space()
{
	//數量是128*128 = 16384
        address_space_array = (struct address_space*)page_malloc(16384 * sizeof(struct address_space));
        address_space_index = 0;
}

void init_radix_allocator()
{
	radix_buffer =
		any_malloc(RADIX_SIZE_LV2 * sizeof(struct radix_tree) / RADIX_THRESHOLD);
	radix_buffer_index = 0;
	radix_buffer_cur_max = RADIX_SIZE_LV2 / RADIX_THRESHOLD;
}

struct radix_tree *alloc_radix_tree()
{
	if (radix_buffer_index >= radix_buffer_cur_max) {
		// 到這裡代表超過上次分配的大小
		printf("Exceed, now: %d, max: %d\n", radix_buffer_index, radix_buffer_cur_max);
		(struct radix_tree*)(radix_buffer + radix_buffer_index) =
			any_malloc(RADIX_SIZE_LV2 * sizeof(struct radix_tree) / RADIX_THRESHOLD);
		radix_buffer_cur_max += RADIX_SIZE_LV2 / RADIX_THRESHOLD;
		printf("Re-allocate, now: %d, max: %d\n", radix_buffer_index, radix_buffer_cur_max);
	}
	return radix_buffer[radix_buffer_index++];
}

int main()
{
	init_radix_allocator();
	int i;
	struct radix_tree *ra;
	for (i = 0; i < 4100; ++i) {
		ra = alloc_radix_tree();
		if (ra == NULL) {
			printf("Allocate %d failed!\n", i);
		}
	}
	return 0;
}

