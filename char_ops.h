#ifndef __CHAR_OPS_H__
#define __CHAR_OPS_H__

#include <ctype.h>
#include <stdio.h>

#define TODIGIT(c) (0x0f & (c))
#define TOLOWER(c) (0x20 | (c))
#define TOUPPER(c) (0xdf & (c))
#define TOINDEX(c) (0x1f & (c))
// #define ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')
// #define ISLOWER(c) ((c) >= 'a' && (c) <= 'z')
#define ISUPPER(c) ((unsigned) ((c) - 'A') <= 25u)
#define ISLOWER(c) ((unsigned) ((c) - 'a') <= 25u)
#define ISALPHA(c) ((ISLOWER(c)) || (ISUPPER(c)))
// #define ISALPHA(c) isalpha(c)
#define ISWHITE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')

typedef enum { Ok, Ff, Eof } skip_stat_t;
skip_stat_t skip(FILE *);

#endif