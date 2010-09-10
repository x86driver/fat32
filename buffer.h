#ifndef _BUFFER_H
#define _BUFFER_H

#include "fat32.h"

void direct_read_sector(void *buf, unsigned int sector);
void direct_read(void *buf, unsigned int cluster);
struct address_space *bread_sector(unsigned int sector);
struct address_space *bread(unsigned int cluster);
void init_all();

#define MAX_FD 8
extern struct inode fd_pool[MAX_FD];

static inline struct address_space *bread_cluster(unsigned int cluster)
{
        unsigned int sector = fat_get_sec(cluster);
        return bread_sector(sector);
}

#endif

