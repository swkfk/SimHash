#include "freq_sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// freq_t freqs[1048576];
int idx[1048576];
int freq_sze;

trie_ndoe_article_t *freqs[1048576];

// int freq_cmp(const void *pa, const void *pb) {
//     freq_t *a = (freq_t *) pa;
//     freq_t *b = (freq_t *) pb;
//     if (a->freq == b->freq) {
//         return strcmp(a->word, b->word);
//     }
//     return b->freq - a->freq;
// }

int freq_cmp(const void *pa, const void *pb) {
    trie_ndoe_article_t *a = *(trie_ndoe_article_t **) pa;
    trie_ndoe_article_t *b = *(trie_ndoe_article_t **) pb;
    if (a->count == b->count) {
        return -1;
    }
    return b->count - a->count;
}

int freq_cmp_idx(const void *pa, const void *pb) {
    trie_ndoe_article_t *a = freqs[*(int *) pa];
    trie_ndoe_article_t *b = freqs[*(int *) pb];
    if (a->count == b->count) {
        return -1;
    }
    return b->count - a->count;
}

// void get_sorted_feature(trie_ndoe_t *root) {
//     walk_trie_tree(root);
//     qsort(freqs, freq_sze, sizeof(freq_t), freq_cmp);
// }

void get_sorted_feature_article(trie_ndoe_article_t *root) {
    walk_trie_tree_article(root);
    qsort(freqs, freq_sze, sizeof(trie_ndoe_article_t *), freq_cmp);
}

static char buf[4096];
static int buf_sze;

// void walk_trie_tree(trie_ndoe_t *root) {
//     if (root->count) {
//         memcpy(freqs[freq_sze].word, buf, buf_sze);
//         freqs[freq_sze].word[buf_sze] = '\0';
//         freqs[freq_sze].freq = root->count;
//         ++freq_sze;
//     }
//     for (int i = 1; i <= 26; ++i) {
//         buf[buf_sze++] = (char) ('a' + i - 1);
//         if (root->next[i]) {
//             walk_trie_tree(root->next[i]);
//         }
//         buf[--buf_sze] = '\0';
//     }
// }

void walk_trie_tree_article(trie_ndoe_article_t *root) {
    if (root->count) {
        freqs[freq_sze++] = root;
    }
    for (int i = 1; i <= 26; ++i) {
        buf[buf_sze++] = (char) ('a' + i - 1);
        if (root->next[i]) {
            walk_trie_tree_article(root->next[i]);
        }
        buf[--buf_sze] = '\0';
    }
}
