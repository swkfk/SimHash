#include "tire.h"
#include "char_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

tire_ndoe_t *new_tire_node() {
    tire_ndoe_t *node = (tire_ndoe_t *) malloc(sizeof(tire_ndoe_t));
    memset(node, 0, sizeof(tire_ndoe_t));
    return node;
}

void insert_word(tire_ndoe_t **proot, const char *word) {
    if (*proot == NULL) {
        *proot = new_tire_node();
    }
    tire_ndoe_t *tra = *proot;
    const char *c = word;
    do {
        if (!tra->next[TOINDEX(*c)]) {
            tra->next[TOINDEX(*c)] = new_tire_node();
        }
        tra = tra->next[TOINDEX(*c)];
    } while (*++c);
    ++tra->count;
}

int find_word(tire_ndoe_t *root, const char *word) {
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
