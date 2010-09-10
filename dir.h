#ifndef _DIR_H
#define _DIR_H

#include "mm.h"
#include "fat32.h"

void init_fat();
int fat_get_entry(struct address_space **addr, struct dir_entry **de);
int fat_parse_long(struct address_space **addr, struct dir_entry **de, char *search_name, int fd);

#endif

