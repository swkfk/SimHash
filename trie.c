#include "trie.h"
#include "char_ops.h"
#include "mempool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t sze_node = sizeof(trie_ndoe_t);
const size_t sze_article = sizeof(trie_ndoe_article_t);
const size_t sze_article_count = 32768 * sizeof(int);

trie_ndoe_t *new_tire_node() {
    // trie_ndoe_t *node = (trie_ndoe_t *) malloc(sizeof(trie_ndoe_t));
    trie_ndoe_t *node = (trie_ndoe_t *) MEMALLOC(sze_node);
    memset(node, 0, sze_node);
    return node;
}

void insert_word(trie_ndoe_t **proot, const char *word) {
    if (*proot == NULL) {
        *proot = new_tire_node();
    }
    trie_ndoe_t *tra = *proot;
    const char *c = word;
    do {
        if (!tra->next[TOINDEX(*c)]) {
            tra->next[TOINDEX(*c)] = new_tire_node();
        }
        tra = tra->next[TOINDEX(*c)];
    } while (*++c);
    ++tra->count;
}

trie_ndoe_article_t *new_tire_article_node() {
    trie_ndoe_article_t *node = (trie_ndoe_article_t *) MEMALLOC(sze_node);
    memset(node, 0, sze_article);
    node->article_cnt = ask_mem_article_count();
    memset(node->article_cnt, 0, 32768 * sizeof(int));
    return node;
}

void insert_article_word(trie_ndoe_article_t **proot, const char *word, int article_idx) {
    if (*proot == NULL) {
        *proot = new_tire_article_node();
    }
    trie_ndoe_article_t *tra = *proot;
    const char *c = word;
    do {
        if (!tra->next[TOINDEX(*c)]) {
            tra->next[TOINDEX(*c)] = new_tire_article_node();
        }
        tra = tra->next[TOINDEX(*c)];
    } while (*++c);
    ++tra->article_cnt[article_idx];
    ++tra->count;
}

int find_word(trie_ndoe_t *root, const char *word) {
    const char *c = word;
    do {
        if (root->next[TOINDEX(*c)]) {
            root = root->next[TOINDEX(*c)];
        } else {
            return 0;
        }
    } while (*++c);
    return root->count;
}

int find_word_in_article_whole(trie_ndoe_article_t *root, const char *word) {
    const char *c = word;
    do {
        if (root->next[TOINDEX(*c)]) {
            root = root->next[TOINDEX(*c)];
        } else {
            return 0;
        }
    } while (*++c);
    return root->count;
}

int find_word_in_article_single(trie_ndoe_article_t *root, const char *word, int article_idx) {
    const char *c = word;
    do {
        if (root->next[TOINDEX(*c)]) {
            root = root->next[TOINDEX(*c)];
        } else {
            return 0;
        }
    } while (*++c);
    return root->article_cnt[article_idx];
}
