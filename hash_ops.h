#ifndef __HASH_OPS_H__
#define __HASH_OPS_H__

#include <stdint.h>

typedef unsigned __int128 hash_t;

#define USE_INT_HASH

#ifdef USE_INT_HASH
extern int hash[][10000];
#else
extern hash_t hashes[];
#endif

void read_hash_value(int, int);
int hamming(unsigned __int128, int);

void print_u128(hash_t x);

#endif