#include "trie.h"
#include "char_ops.h"
#include "consts.h"
#include "mempool.h"
#include "stop_ops.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const size_t sze_node = sizeof(trie_ndoe_t);
const size_t sze_article = sizeof(trie_ndoe_article_t);
const size_t sze_article_count = ARTICLE_CNT * sizeof(int);

trie_ndoe_t *new_tire_node() {
    // trie_ndoe_t *node = (trie_ndoe_t *) malloc(sizeof(trie_ndoe_t));
    trie_ndoe_t *node = (trie_ndoe_t *) ask_mem(sze_node);
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
    trie_ndoe_article_t *node = (trie_ndoe_article_t *) ask_mem(sze_article);
    // memset(node, 0, sze_article);
    // node->article_cnt = (int *) ask_mem_article_count();
    // node->article_cnt = calloc(1, sze_article_count);
    // memset(node->article_cnt, 0, sze_article_count);
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
    if (!tra->article_cnt) {
        tra->article_cnt = (int *) ask_mem_article_count();
        // assert(tra->article_cnt);
    }
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

// ======== Avoid double read of the articles ========

int cur_article_id, char_idx;
trie_ndoe_article_t *cur_node, *cur_root;
// trie_ndoe_t *cur_stop_node, *cur_stop_root;

void new_article(int art_idx, trie_ndoe_article_t **proot) {
    if (*proot == NULL) {
        *proot = new_tire_article_node();
    }
    cur_article_id = art_idx;
    cur_root = cur_node = *proot;
    // cur_stop_root = cur_stop_node = stop_root;
}

void insert_char(int chr) {
    char_idx = TOINDEX(chr);
    if (!cur_node->next[char_idx]) {
        cur_node = cur_node->next[char_idx] = new_tire_article_node();
    } else {
        cur_node = cur_node->next[char_idx];
    }
    // if (cur_stop_node && cur_stop_node->next[char_idx]) {
    //     cur_stop_node = cur_stop_node->next[char_idx];
    // } else {
    //     cur_stop_node = NULL;
    // }
}

void finish_word() {
    if (cur_node->count == -1) {
        cur_node = cur_root;
        return;
    }
    if (!cur_node->article_cnt) {
        cur_node->article_cnt = (int *) ask_mem_article_count();
        // assert(cur_node->article_cnt);
    }
    // if (!cur_stop_node || !cur_stop_node->count) {
    ++cur_node->article_cnt[cur_article_id];
    ++cur_node->count;
    // }
    cur_node = cur_root;
    // cur_stop_node = cur_stop_root;
}

void finish_stop() {
    if (!cur_node->article_cnt) {
        cur_node->article_cnt = (int *) ask_mem_article_count();
        // assert(cur_node->article_cnt);
    }
    // if (!cur_stop_node || !cur_stop_node->count) {
    // ++cur_node->article_cnt[cur_article_id];
    cur_node->count = -1;
    // }
    cur_node = cur_root;
    // cur_stop_node = cur_stop_root;
}