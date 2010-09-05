#ifndef _RADIX_TREE_H
#define _RADIX_TREE_H

#define RADIX_SIZE 128

struct radix_tree {
	struct radix_tree *next[RADIX_SIZE];
};

#endif

