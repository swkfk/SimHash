#ifndef __PATCH_IO_H__
#define __PATCH_IO_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern char passage_buf[];
extern uint_fast32_t passage_len;

void open_write_handle(const char *);
void println(const char *);
void prints(const char *);
void flush();
void close_io_handle();

#define printc(c) (passage_buf[passage_len++] = (c))
#define endl() (passage_buf[passage_len++] = '\n')

#endif