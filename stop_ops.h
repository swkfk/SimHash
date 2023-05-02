#ifndef __STOP_OPS_H__
#define __STOP_OPS_H__

#include "trie.h"

void read_stop_words();
int is_stop_word(const char *);

extern trie_ndoe_t *stop_root;

#endif