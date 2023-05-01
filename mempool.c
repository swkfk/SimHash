#include "mempool.h"
#include "trie.h"
#include <stdio.h>

// ======== Mem pool ========

pool_t pool = {.count = PIECE_RESERVED, .used = 0, .start = NULL};
pool_t pool_article = {.count = PIECE_RESERVED >> 8, .used = 0, .start = NULL};

void new_pool(size_t offset, pool_t *p) {
    p->offset = offset;
    p->used = 0;
    p->start = calloc(p->count, offset);
}

void *ask_mem(size_t sze) {
    if (pool.used == pool.count) {
        new_pool(sze, &pool);
    }
    return pool.start + (pool.used++ * pool.offset);
}

void *ask_mem_article_count() {
    if (pool_article.used == pool_article.count) {
        new_pool(sze_article_count, &pool_article);
    }
    return pool_article.start + (pool_article.used++ * pool_article.offset);
}

// ======== Try to speed up ========

// #include "trie.h"
// #include <stdio.h>

// const size_t offset = sizeof(trie_ndoe_t);
// char mem_pool[sizeof(trie_ndoe_t) * PIECE_RESERVED * 8];

// void *ask_mem_from_fake_pool(size_t sze) {
//     if (pool.used == pool.count) {
//         pool.used = 0;
//     }
//     return ((void *) mem_pool) + (pool.used++ * offset);
// }
