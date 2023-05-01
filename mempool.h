#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include <stddef.h>
#include <stdlib.h>

#define PIECE_RESERVED 1048576

#define MEMALLOC ask_mem

typedef struct {
    size_t offset;
    size_t count;
    size_t used;
    void *start;
} pool_t;

void new_pool(size_t);
void *ask_mem(size_t);

void *ask_mem_from_fake_pool(size_t);

#endif