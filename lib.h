#ifndef _LIB_H
#define _LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_MEMORY_USAGE
#define DEBUG

#ifdef DEBUG
#define BUG_ON(cond) do { if (cond) { printf("BUG ON %p, %s\n", __builtin_return_address(0), __FUNCTION__); exit(1);} } while (0);
#else
#define BUS_ON(cond)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


#endif

