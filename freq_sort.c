#include "freq_sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

freq_t freqs[1048576];
int freq_sze;

int freq_cmp(const void *pa, const void *pb) {
    freq_t *a = (freq_t *) pa;
    freq_t *b = (freq_t *) pb;
    if (a->freq == b->freq) {
        return strcmp(a->word, b->word);
    }
    return b->freq - a->freq;
}

void get_sorted_feature(tire_ndoe_t *root) {
    walk_tire_tree(root);
    qsort(freqs, freq_sze, sizeof(freq_t), freq_cmp);
    FILE *fp = fopen("./feature.old.txt", "w");
    for (int i = 0; i < 1000; ++i) {
        fprintf(fp, "%s %d\n", freqs[i].word, freqs[i].freq);
    }
    fclose(fp);
}

static char buf[4096];
static int buf_sze;

void walk_tire_tree(tire_ndoe_t *root) {
    if (root->count) {
        memcpy(freqs[freq_sze].word, buf, buf_sze);
        freqs[freq_sze].word[buf_sze] = '\0';
        freqs[freq_sze].freq = root->count;
        ++freq_sze;
    }
    for (int i = 1; i <= 26; ++i) {
        buf[buf_sze++] = (char) ('a' + i - 1);
        if (root->next[i]) {
            walk_tire_tree(root->next[i]);
        }
        buf[--buf_sze] = '\0';
    }
}
