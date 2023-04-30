#ifndef __FREQ_SORT_H__
#define __FREQ_SORT_H__

#include "tire.h"

typedef struct {
    char word[64];
    int freq;
} freq_t;

extern freq_t freqs[1048576];
extern int freq_sze;

// In this version, I use the `qsort`
int freq_cmp(const void *, const void *);
void get_sorted_feature(tire_ndoe_t *);

void walk_tire_tree(tire_ndoe_t *);

#endif