#ifndef __PATCH_IO_H__
#define __PATCH_IO_H__

#include <stdio.h>

#define PATCH_BUF_SZE 2097152

typedef struct {
    FILE *stream;
    char off; // buf[-1]
    char buf[PATCH_BUF_SZE];
    int pos;
    int nbyte;
} patch_io_t;

void open_read_handle(const char *);
void open_write_handle(const char *);
void close_io_handle();

int ask_char();
int ask_word(char *);
int ask_string(char *);
void un_char(int);

int skip_white();
int skip_noalpha();

void println(const char *);
void prints(const char *);
void printc(const char);
void endl();
void flush();

void patch_read_();
void patch_write_();

#endif