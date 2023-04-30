#include "char_ops.h"
#include <stdio.h>

skip_stat_t skip(FILE *stream) {
    int chr;
    while ((chr = fgetc(stream))) {
        if (chr == EOF) {
            return Eof;
        }
        if (chr == '\f') {
            return Ff;
        }
        if (ISALPHA(chr)) {
            ungetc(chr, stream);
            return Ok;
        }
    }
    return Ok;
}
