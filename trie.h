#ifndef __TRIE_H__
#define __TRIE_H__

#include <stddef.h>

typedef struct trie_node {
    struct trie_node *next[27];
    int count;
} trie_ndoe_t;

typedef struct trie_node_article {
    struct trie_node_article *next[27];
    int count;
    int *article_cnt;
} trie_ndoe_article_t;

trie_ndoe_t *new_tire_node();
void insert_word(trie_ndoe_t **, const char *);
int find_word(trie_ndoe_t *, const char *);

trie_ndoe_article_t *new_tire_article_node();
void insert_article_word(trie_ndoe_article_t **, const char *, int);
int find_word_in_article_whole(trie_ndoe_article_t *, const char *);
int find_word_in_article_single(trie_ndoe_article_t *, const char *, int);

void new_article(int, trie_ndoe_article_t **);
void insert_char(int);
void finish_word();

extern const size_t sze_node;
extern const size_t sze_article;
extern const size_t sze_article_count;

#endif