#include "page.h"
#include "mm.h"

struct address_space *address_space_array;
unsigned int address_space_index;

void init_address_space()
{
	//數量是128*128 = 16384
        address_space_array = (struct address_space*)page_malloc(16384 * sizeof(struct address_space));
        address_space_index = 0;
}
