#ifndef _RADIX_TREE_H
#define _RADIX_TREE_H

struct radix_tree {
	struct radix_tree *next[128];
};

#endif

