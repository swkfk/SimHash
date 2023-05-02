#ifndef __HASH_OPS_H__
#define __HASH_OPS_H__

#include <stdint.h>

typedef unsigned __int128 hash_t;

extern hash_t hashes[];
// extern int hash[][128];

void read_hash_value(int, int);
int hamming(unsigned __int128, int);
#endif