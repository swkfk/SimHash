#ifndef __ARRAY_H__
#define __ARRAY_H__

#include "consts.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t elem_size;
    size_t capacity;
    size_t size;
    void *dat;
} array_t;

array_t init_array(size_t elem_size, size_t init_cap);
size_t append_array(array_t *pa, void *pelem);
size_t size_array(array_t *pa);
void expand_array(array_t *pa);
void set_array(array_t *pa, void *pelem, size_t index);
void *get_array(array_t *pa, size_t index);

#endif