#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__

#include <stddef.h>
#include <stdlib.h>

#define PIECE_RESERVED 1048576
// #define PIECE_RESERVED 65536

typedef struct {
    size_t offset;
    size_t count;
    size_t used;
    void *start;
} pool_t;

void new_pool(size_t, pool_t *);
void *ask_mem(size_t);

void *ask_mem_article_count();

extern pool_t pool;
extern pool_t pool_article;

#endif