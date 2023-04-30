#include "hash_ops.h"
#include <stdio.h>

hash_t hashes[10000];

static char buf[512];

void print_u128(hash_t x) {
    if (x > 0) {
        print_u128(x >> 1);
        putchar((int) ((x & 1) + '0'));
    }
}

void read_hash_value(int rown, int coln) {
    FILE *stream = fopen("hashvalue.txt", "r");
    for (int row = 0; row < rown; ++row) {
        fgets(buf, 512, stream);
        for (int i = 0; i < coln; ++i) {
            hashes[row] = hashes[row] << 1 | (buf[i] == '1');
        }
    }
    fclose(stream);
}

int hamming(unsigned __int128 src, int n) {
    int res = 0;
    while (n--) {
        res += (int) (src & 1);
        src >>= 1;
    }
    return res;
}