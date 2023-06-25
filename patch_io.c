#include "patch_io.h"
#include "consts.h"

// char passage_buf[204800];
// uint_fast32_t passage_len;
FILE *fp;

void open_write_handle(const char *__restrict name) {
    fp = fopen(name, "wb");
    // passage_buf[passage_len = 0] = 0;
}

void println(const char *__restrict s) {
    // while (*s) {
    //     passage_buf[passage_len++] = *(s++);
    // }
    // passage_buf[passage_len++] = '\n';
    fputs(s, fp);
    fputc('\n', fp);
}

void prints(const char *__restrict s) {
    fputs(s, fp);
}

void printc(const char c) {
    fputc(c, fp);
}

void endl() {
    fputc('\n', fp);
}

void flush() {
    // fwrite(passage_buf, 1, passage_len, fp);
}

void close_io_handle() {
    fclose(fp);
}
