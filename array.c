#include "array.h"

array_t init_array(size_t elem_size, size_t init_cap) {
    array_t res = {.size = 0, .elem_size = elem_size, .capacity = init_cap};
    res.dat = calloc(init_cap, elem_size);
    return res;
}

// void set_array(array_t *pa, void *pelem, size_t index) {
//     while (index >= pa->capacity) {
//         expand_array(pa);
//     }
//     memcpy(pa->dat + index * pa->elem_size, pelem, pa->elem_size);
// }

// void *get_array(array_t *pa, size_t index) {
//     assert(index < pa->capacity);
//     return pa->dat + index * pa->elem_size;
// }

size_t size_array(array_t *pa) {
    return pa->size;
}

size_t append_array(array_t *pa, void *pelem) {
    if (pa->size + 1 >= pa->capacity) {
        expand_array(pa);
    }
    memcpy(pa->dat + pa->size * pa->elem_size, pelem, pa->elem_size);
    return pa->size++; // Return the index
}

void expand_array(array_t *pa) {
#ifdef DEBUG_PRINT_ARRAY_APPEND
    printf("<Extend: from %zd to %zd>\n", pa->capacity, pa->capacity * 5 / 4 + 1);
#endif
    size_t ori = pa->capacity;
    pa->capacity = pa->capacity * 5 / 4 + 1;
    pa->dat = realloc(pa->dat, pa->capacity * pa->elem_size);
    assert(pa->dat != NULL);
    memset(pa->dat + ori * pa->elem_size, 0, pa->capacity * pa->elem_size - ori * pa->elem_size);
}
