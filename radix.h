#ifndef _RADIX_TREE_H
#define _RADIX_TREE_H

#include "mm.h"

#define RADIX_SIZE 128
#define FIND_NODE 0
#define NEW_NODE 1

struct radix_tree {
	struct radix_tree *next[RADIX_SIZE];
};

extern struct radix_tree *radix;

struct address_space *find_or_create(struct radix_tree * restrict radix,
                unsigned int cluster, int * restrict create);

static inline struct address_space *lookup(unsigned int cluster, int * restrict create)
{
        return find_or_create(radix, cluster, create);
}

#endif

