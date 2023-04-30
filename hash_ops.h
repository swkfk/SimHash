#ifndef __HASH_OPS_H__
#define __HASH_OPS_H__

typedef unsigned __int128 hash_t;

extern hash_t hashes[];

void read_hash_value(int, int);
int hamming(unsigned __int128, int);

#endif