#ifndef __TIRE_H__
#define __TIRE_H__

typedef struct tire_node {
    struct tire_node *next[27];
    int count;
} tire_ndoe_t;

tire_ndoe_t *new_tire_node();
void insert_word(tire_ndoe_t **, const char *);
int find_word(tire_ndoe_t *, const char *);

#endif