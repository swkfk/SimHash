#include "mempool.h"

// ======== Mem pool ========

pool_t pool = {.count = PIECE_RESERVED, .used = PIECE_RESERVED, .start = NULL};

void new_pool(size_t offset) {
    pool.offset = offset;
    pool.used = 0;
    pool.start = malloc(offset * PIECE_RESERVED);
}

void *ask_mem(size_t sze) {
    if (pool.used == pool.count) {
        new_pool(sze);
    }
    return pool.start + (pool.used++ * pool.offset);
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
