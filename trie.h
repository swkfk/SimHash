#ifndef __TRIE_H__
#define __TRIE_H__

typedef struct trie_node {
    struct trie_node *next[27];
    int count;
} trie_ndoe_t;

trie_ndoe_t *new_tire_node();
void insert_word(trie_ndoe_t **, const char *);
int find_word(trie_ndoe_t *, const char *);

#endif