#include "patch_io.h"
#include "char_ops.h"
#include "trie.h"

#include <stdio.h>

static patch_io_t handle;

void open_read_handle(const char *file_name) {
    handle.stream = fopen(file_name, "r");
    patch_read_();
}

void open_write_handle(const char *file_name) {
    handle.stream = fopen(file_name, "w");
    handle.pos = 0;
}

void close_io_handle() {
    fclose(handle.stream);
    handle.stream = NULL;
    handle.pos = 0;
}

// int ask_char() {
//     // if (handle.pos == handle.nbyte) {
//     //     // if (handle.nbyte < PATCH_BUF_SZE) {
//     //     //     return EOF;
//     //     // }
//     //     // patch_read_();
//     // }
//     return handle.buf[handle.pos++];
// }

int ask_word(char *out) {
    int chr;
    while (chr = ask_char(), ISALPHA(chr)) {
        *(out++) = (char) chr;
    }
    *out = '\0';
    if (chr == '\f') {
        return 1;
    } else if (chr == EOF) {
        return EOF;
    } else {
        return 0;
    }
}

void read_word() {
    int chr;
    while (chr = ask_char(), ISALPHA(chr)) {
        insert_char(chr);
    }
    finish_word();
}

int ask_string(char *out) {
    int ret;
    if ((ret = skip_white())) {
        return ret;
    }
    while (ret = ask_char(), !ISWHITE(ret)) {
        *(out++) = (char) ret;
    }
    *out = '\0';
    if (ret == '\f') {
        return 1;
    } else if (ret == EOF) {
        return EOF;
    } else {
        return 0;
    }
}

void un_char(int c) {
    handle.buf[--handle.pos] = (char) c;
}

int skip_white() {
    int chr;
    while (chr = ask_char(), ISWHITE(chr))
        ;
    if (chr == EOF) {
        return -1;
    }
    if (chr == '\f') {
        return 1;
    }
    un_char(chr);
    return 0;
}

int skip_noalpha() {
    int chr;
    for (;;) {
        chr = ask_char();
        if (chr == EOF) {
            return -1;
        } else if (chr == '\f') {
            return 1;
        } else if (ISALPHA(chr)) {
            un_char(chr);
            return 0;
        }
    }
}

void println(const char *s) {
    prints(s);
    endl();
}

void prints(const char *s) {
    while (*s) {
        handle.buf[handle.pos++] = *(s++);
        if (handle.pos == PATCH_BUF_SZE) {
            patch_write_();
        }
    }
}

void printc(const char c) {
    handle.buf[handle.pos++] = c;
    if (handle.pos == PATCH_BUF_SZE) {
        patch_write_();
    }
}

void endl() {
    printc('\n');
}

void flush() {
    if (handle.pos) {
        patch_write_();
    }
}

void patch_read_() {
    handle.pos = 0;
    handle.nbyte = (int) fread(handle.buf, 1, PATCH_BUF_SZE, handle.stream);
    handle.buf[handle.nbyte] = EOF;
}

void patch_write_() {
    fwrite(handle.buf, 1, handle.pos, handle.stream);
    handle.pos = 0;
}
