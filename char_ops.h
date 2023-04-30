#ifndef __CHAR_OPS_H__
#define __CHAR_OPS_H__

#include <stdio.h>

#define TODIGIT(c) (0x0f & (c))
#define TOLOWER(c) (0x20 | (c))
#define TOUPPER(c) (0xdf & (c))
#define TOINDEX(c) (0x1f & (c))
#define ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define ISALPHA(c) ((ISLOWER(c)) || (ISUPPER(c)))

typedef enum { Ok, Ff, Eof } skip_stat_t;
skip_stat_t skip(FILE *);

#endif