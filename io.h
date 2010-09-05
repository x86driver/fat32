#ifndef _IO_H
#define _IO_H

void init_disk();
void read8blocks(void *buf, unsigned int start_block);

#endif

