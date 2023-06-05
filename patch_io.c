#include "patch_io.h"
#include "consts.h"

char passage_buf[PATCH_BUF_SZE];
uint_fast32_t passage_len;
FILE *fp;

void open_write_handle(const char *__restrict name) {
    fp = fopen(name, "wb");
    passage_buf[passage_len = 0] = 0;
}

void println(const char *__restrict s) {
    while (*s) {
        passage_buf[passage_len++] = *(s++);
    }
    passage_buf[passage_len++] = '\n';
}

void prints(const char *__restrict s) {
    while (*s) {
        passage_buf[passage_len++] = *(s++);
    }
}

void printc(const char c) {
    passage_buf[passage_len++] = c;
}

void endl() {
    passage_buf[passage_len++] = '\n';
}

void flush() {
    fwrite_unlocked(passage_buf, 1, passage_len, fp);
}

void close_io_handle() {
    fclose(fp);
}
