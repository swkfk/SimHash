#ifndef __FREQ_SORT_H__
#define __FREQ_SORT_H__

#include "trie.h"

typedef struct {
    char word[64];
    int freq;
} freq_t;

// extern freq_t freqs[1048576];
extern trie_ndoe_article_t *freqs[1048576];
extern int idx[];
// extern int freq_sze;

// In this version, I use the `qsort`
int freq_cmp(const void *, const void *);
// void get_sorted_feature(trie_ndoe_t *);
void get_sorted_feature_article(trie_ndoe_article_t *);

// void walk_trie_tree(trie_ndoe_t *);
void walk_trie_tree_article(trie_ndoe_article_t *);
void stack_preorder(trie_ndoe_article_t *);
#endif