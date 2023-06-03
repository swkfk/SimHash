#include "stop_ops.h"
#include "trie.h"

#include <stdio.h>

trie_ndoe_t *stop_root;

void read_stop_words() {
    FILE *stop_in = fopen("stopwords.txt", "rb");
    char /* *start,*/ *end;
    char buf[4096];
    fread(buf, 1, 4096, stop_in);
    /*start =*/end = buf;
    while (*end) {
        if (*end == '\r') {
            // *end = '\0';
        } else if (*end == '\n') {
            // *end = '\0';
            // insert_word(&stop_root, start);
            finish_stop();
            // start = end + 1;
        } else {
            insert_char(*end);
        }
        ++end;
    }
    fclose(stop_in);
}

// int is_stop_word(const char *s) {
//     return find_word(stop_root, s);
// }
