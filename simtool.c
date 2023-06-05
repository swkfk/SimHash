
#pragma GCC optimize("Ofast,no-stack-protector,unroll-loops,fast-math")
#pragma GCC target("fma,sse,sse2,sse3,ssse3,sse4.1,sse4.2,avx,avx2,popcnt,tune=native")
#pragma GCC optimize(3, "Ofast", "inline")

#include <immintrin.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// #define DEBUG

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "char_ops.h"
#include "consts.h"
#include "hash_ops.h"
#include "patch_io.h"

#ifdef DEBUG
#define printf_d printf
#else
#define printf_d(...) ;
#endif

typedef unsigned __int128 finger_t;

int str_to_int(const char *s) {
    int res = 0;
    const char *p = s;
    do {
        res = res * 10 + TODIGIT(*p);
    } while (*++p);
    return res;
}

typedef union {
    __uint128_t u128;
    uint64_t u64[2];
} u128_t;

static inline uint_fast8_t popcnt_u128(__uint128_t n) {
    const u128_t u = {.u128 = n};
    const uint_fast8_t cnt_a = __builtin_popcountll(u.u64[0]);
    const uint_fast8_t cnt_b = __builtin_popcountll(u.u64[1]);
    return cnt_a + cnt_b;
}

int vector_length, finger_length, patch_size;
// trie_ndoe_article_t *total_root;
// trie_ndoe_t *sample_root[SAMPLE_CNT];

char article_ids[ARTICLE_CNT][64];
char *article_starts[ARTICLE_CNT], *article_ends[ARTICLE_CNT];
int article_sze;
finger_t article_fingers[ARTICLE_CNT];

char sample_ids[SAMPLE_CNT][64];
int sample_sze;
finger_t sample_fingers[SAMPLE_CNT];

// int web_weight[10000]; // tmp, each article & sample
int *web_weight;

// the distance for each sample
int hamming_0[ARTICLE_CNT];
int hamming_0_sze;
int hamming_1[ARTICLE_CNT];
int hamming_1_sze;
int hamming_2[ARTICLE_CNT];
int hamming_2_sze;
int hamming_3[ARTICLE_CNT];
int hamming_3_sze;

int word_count[ARTICLE_CNT][10000] /*__attribute__((__aligned__(32)))*/;

unsigned int tmp;
unsigned __int128 tmp128;

// __m256i avx_a, avx_b, avx_c;
// int32_t avx_arr[8] __attribute__((__aligned__(32)));

typedef struct _TRIE_NODE {
    uint_fast32_t count;
    int next[27];
} array_trie_node_t;

array_trie_node_t trie_tree[300000];
uint_fast32_t cur_trie_idx, next_alloc_idx = 1;

void read_stop() {
    FILE *fp = fopen("./stopwords.txt", "rb");
    passage_buf[fread_unlocked(passage_buf, 1, PATCH_BUF_SZE, fp)] = '\n';
    fclose(fp);
    register char *c = passage_buf;
    while (*c) {
        if (*c == '\r')
            ;
        else if (*c == '\n') {
            trie_tree[cur_trie_idx].count = 0x80000000u;
            cur_trie_idx = 0;
        } else {
            if (!trie_tree[cur_trie_idx].next[TOINDEX(*c)]) {
                trie_tree[cur_trie_idx].next[TOINDEX(*c)] = next_alloc_idx++;
            }
            cur_trie_idx = trie_tree[cur_trie_idx].next[TOINDEX(*c)];
        }
        ++c;
    }
    if (cur_trie_idx) {
        trie_tree[cur_trie_idx].count = 0x80000000u;
        cur_trie_idx = 0;
    }
}

void read_whole_articles() {
    FILE *fp = fopen("./article.txt", "rb");
    passage_len = fread_unlocked(passage_buf, 1, PATCH_BUF_SZE, fp);
    fclose(fp);
    register char *c = passage_buf;
    register uint_fast32_t tmp_idx;
    while (*c) {
        while (*c == '\f' || *c == '\r' || *c == '\n') {
            ++c;
        }
        if (passage_len == c - passage_buf) {
            break;
        }
        tmp_idx = 0;
        while (*c != '\r' && *c != '\n') {
            article_ids[article_sze][tmp_idx++] = *(c++);
        }
        article_starts[article_sze] = c;
        ++article_sze;

        for (;;) {
            if (ISALPHA(*c)) {
                if (!trie_tree[cur_trie_idx].next[TOINDEX(*c)]) {
                    trie_tree[cur_trie_idx].next[TOINDEX(*c)] = next_alloc_idx++;
                }
                cur_trie_idx = trie_tree[cur_trie_idx].next[TOINDEX(*c)];
                ++c;
            } else {
                // if (!(trie_tree[cur_trie_idx].count >> 31)) {
                //    // not stop words
                ++trie_tree[cur_trie_idx].count;
                // }
                cur_trie_idx = 0;
                while (!ISALPHA(*c)) {
                    if (*c == '\f' || !*c) {
                        article_ends[article_sze - 1] = c;
                        goto end_of_endless_loop;
                    }
                    *(c++) = '\0';
                }
            }
        }
    end_of_endless_loop:;
    }
}

typedef struct {
    uint_fast32_t idx, count;
} mosts_t;
mosts_t mosts[WORD_CNT];
uint_fast32_t word_sze;
uint_fast32_t stack[WORD_CNT];
uint_fast32_t stack_sze;

void walk_trie() {
    // for (int i = 1; i < next_alloc_idx; ++i) {
    //     // plus 1 is important
    //     if (trie_tree[i].count + 1 > 1) {
    //         mosts[word_sze].idx = i;
    //         mosts[word_sze].count = trie_tree[i].count;
    //         ++word_sze;
    //     }
    // }
    uint32_t root;
    stack[stack_sze++] = 0;
    while (stack_sze) {
        root = stack[--stack_sze];
        if (!(trie_tree[root].count >> 31)) {
            mosts[word_sze].idx = root;
            mosts[word_sze].count = trie_tree[root].count;
            ++word_sze;
        }
        for (register int i = 26; i >= 1; --i) {
            if (trie_tree[root].next[i]) {
                stack[stack_sze++] = trie_tree[root].next[i];
            }
        }
    }
    // printf("%lu %lu\n", mosts[0].count, mosts[0].idx);
    // exit(0);
    mosts[0].count = 0;
    // for (int i = 0; i < vector_length; ++i) {
    //     printf("%lu ", mosts[i].count);
    // }
    // printf("\n");
}

#ifdef DEBUG_MOST_WORD_PRINT
int print_trie_buf_sze_;
char print_trie_buf_[80];

typedef struct {
    char word[80];
    int count;
} word_t_;
word_t_ words_[10001];
int print_word_sze_;

void print_trie(int root) {
    if (root) {
        for (int j = 0; j < vector_length; ++j) {
            if (mosts[j].idx == root) {
                printf("#%3d: %s %lu\n", print_word_sze_ + 1, print_trie_buf_, mosts[j].count);
                words_[print_word_sze_].count = mosts[j].count;
                strcpy(words_[print_word_sze_].word, print_trie_buf_);
                ++print_word_sze_;
            }
        }
#ifdef DEBUG_CATCH_FREQ_1000
        if (trie_tree[root].count == 1000) {
            printf("Caught 1000! `%s`\n", print_trie_buf_);
            exit(0);
        }
#endif
    }
    for (int i = 1; i <= 26; ++i) {
        if (trie_tree[root].next[i]) {
            print_trie_buf_[print_trie_buf_sze_++] = 'a' + i - 1;
            print_trie(trie_tree[root].next[i]);
            print_trie_buf_[--print_trie_buf_sze_] = 0;
        }
    }
}

int word_cmp_(const void *a, const void *b) {
    return -((word_t_ *) a)->count + ((word_t_ *) b)->count;
}

void print_trie_result() {
    printf("print_word_sze_: %d\n", print_word_sze_);
    qsort(words_, print_word_sze_, sizeof(word_t_), word_cmp_);
    FILE *fp = fopen("./feature.new.txt", "w");
    for (int i = 0; i < print_word_sze_; ++i) {
        fprintf(fp, "%s %d\n", words_[i].word, words_[i].count);
    }
    fclose(fp);
    int idx = trie_tree[0].next[TOINDEX('l')];
    idx = trie_tree[idx].next[TOINDEX('o')];
    idx = trie_tree[idx].next[TOINDEX('o')];
    idx = trie_tree[idx].next[TOINDEX('k')];
    idx = trie_tree[idx].next[TOINDEX('e')];
    idx = trie_tree[idx].next[TOINDEX('d')];
    printf("*** looked: %lu***\n", trie_tree[idx].count);
}

#else
#define print_trie(...) ;
#define print_trie_result(...) ;
#endif

int cmp(const void *a, const void *b) {
    // return memcmp(&(((mosts_t *) a)->count), &(((mosts_t *) b)->count), sizeof(uint_fast32_t));
    // ==: -1, no swap
    return (((mosts_t *) a)->count >= ((mosts_t *) b)->count) ? -1 : 1;
}

// uint_fast32_t count_1[10005], count_2[10005];
// uint_fast32_t *count_ptr[] = {count_1, count_2};
// uint_fast8_t count_cur = 1, count_last = 0;
// uint_fast32_t *cptr_cur, *cptr_last;

void get_article_features() {
    qsort(mosts, word_sze, sizeof(mosts_t), cmp);
    print_trie(0);
    print_trie_result();
    // for (int i = 0; i < vector_length; ++i) {
    //     printf("%lu ", mosts[i].count);
    // }
    // printf("\n");
#ifdef DEBUG_PRINT_MOST_FREQS
    for (int i = 0; i < vector_length; ++i) {
        printf("Most freqs: #%3d: %lu\n", i + 1, mosts[i].count);
    }
#endif
    for (register int j = 0; j < vector_length; ++j) {
        trie_tree[mosts[j].idx].count = 0;
    }
    for (int i = 0; i < article_sze; ++i) {
        // cptr_cur = count_ptr[(count_cur++) & 1];
        // cptr_last = count_ptr[(count_last++) & 1];
        register char *c = article_starts[i], *tend = article_ends[i];
        while (c < tend) {
            if (*c) {
                cur_trie_idx = trie_tree[cur_trie_idx].next[TOINDEX(*c)];
                ++c;
            } else {
                while (!*(++c))
                    ;
                ++trie_tree[cur_trie_idx].count;
                cur_trie_idx = 0;
            }
        }
        for (register int j = 0; j < vector_length; ++j) {
            // word_count[i][j] = cptr_cur[j] - cptr_last[j];
            word_count[i][j] = trie_tree[mosts[j].idx].count;
            trie_tree[mosts[j].idx].count = 0;
        }
    }
}

int *arr_tmp;

void calculate_finger() {
    for (int i = 0; i < article_sze; ++i) {
        web_weight = word_count[i];
        tmp128 = 0;
        for (int finger_bit = 0; finger_bit < finger_length; ++finger_bit) {
            tmp = 0x80000000u;
            arr_tmp = hash[finger_bit];
            for (int i = 0; i < vector_length; ++i) {
                tmp += arr_tmp[i] * web_weight[i];
            }
            // avx_c = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0);
            // for (int i = 0; i < patch_size; ++i) {
            //     avx_a = _mm256_load_epi32(arr_tmp + i * 8);
            //     avx_b = _mm256_load_epi32(web_weight + i * 8);
            //     avx_a = _mm256_mul_epi32(avx_a, avx_b);
            //     avx_c = _mm256_add_epi32(avx_a, avx_c);
            // }
            // _mm256_store_epi32(avx_arr, avx_c);
            // for (int i = 0; i < 7; ++i) {
            //     tmp += avx_arr[i];
            // }
            tmp128 = tmp128 << 1 | (tmp >> 31); // < 0x80000000u => 1
        }
        article_fingers[i] = tmp128;
    }
}

void read_whole_samples() {
    FILE *fp = fopen("./sample.txt", "rb");
    passage_len = fread(passage_buf, 1, PATCH_BUF_SZE - 1, fp);
    fclose(fp);
    register char *c = passage_buf;
    register uint_fast32_t tmp_idx;
    while (*c) {
        while (*c == '\f' || *c == '\r' || *c == '\n') {
#ifdef DEBUG_PRINT_SAMPLE_INFO
            printf("At begin, read `%c`, sze: %d, next: [%c]\n", *c, sample_sze, *(c + 1));
#endif
            ++c;
        }
        if (passage_len == c - passage_buf) {
            break;
        }
        tmp_idx = 0;
        while (*c != '\r' && *c != '\n') {
            sample_ids[sample_sze][tmp_idx++] = *(c++);
        }
        ++sample_sze;

        for (;;) {
            if (ISALPHA(*c)) {
                if (!trie_tree[cur_trie_idx].next[TOINDEX(*c)]) {
                    trie_tree[cur_trie_idx].next[TOINDEX(*c)] = next_alloc_idx++;
                }
                cur_trie_idx = trie_tree[cur_trie_idx].next[TOINDEX(*c)];
                ++c;
            } else {
                ++trie_tree[cur_trie_idx].count;
                cur_trie_idx = 0;
                while (!ISALPHA(*c)) {
                    if (*c == '\f' || !*c) {
#ifdef DEBUG_PRINT_SAMPLE_INFO
                        printf("Jump into! Name: %s\n", sample_ids[sample_sze - 1]);
#endif
                        goto end_of_endless_loop_sample;
                    }
                    // *(c++) = '\0';
                    ++c;
                }
            }
        }
    end_of_endless_loop_sample:
        arr_tmp = word_count[sample_sze - 1];
        for (register int j = 0; j < vector_length; ++j) {
            arr_tmp[j] = trie_tree[mosts[j].idx].count;
            trie_tree[mosts[j].idx].count = 0;
        }
    }
}

void calculate_finger_sample() {
    for (int i = 0; i < sample_sze; ++i) {
        web_weight = word_count[i];
        tmp128 = 0;
        for (int finger_bit = 0; finger_bit < finger_length; ++finger_bit) {
            arr_tmp = hash[finger_bit];
            tmp = 0x80000000u;
            for (int j = 0; j < vector_length; ++j) {
                tmp += web_weight[j] * arr_tmp[j];
            }
            tmp128 = tmp128 << 1 | (tmp >> 31); // < 0x80000000u => 1
        }
        sample_fingers[i] = tmp128;
    }
}

void emit_results() {
    open_write_handle("result.txt");
    int art_idx;
    for (int sam_idx = 0; sam_idx < sample_sze; ++sam_idx) {
        hamming_0_sze = hamming_1_sze = hamming_2_sze = hamming_3_sze = 0;
        for (art_idx = 0; art_idx < article_sze; ++art_idx) {
            tmp = popcnt_u128(article_fingers[art_idx] ^ sample_fingers[sam_idx]);
            if (likely(tmp > 3)) {
                continue;
            }
            if (tmp == 0) {
                hamming_0[hamming_0_sze++] = art_idx;
            } else if (tmp == 1) {
                hamming_1[hamming_1_sze++] = art_idx;
            } else if (tmp == 2) {
                hamming_2[hamming_2_sze++] = art_idx;
            } else if (tmp == 3) {
                hamming_3[hamming_3_sze++] = art_idx;
            }
        }
        if (unlikely(!sam_idx)) {
            puts(sample_ids[sam_idx]);
            if (hamming_0_sze) {
                printf("0:");
                for (int i = 0; i < hamming_0_sze; ++i) {
                    printf("%s ", article_ids[hamming_0[i]]);
                }
                putchar('\n');
            }
            if (hamming_1_sze) {
                printf("1:");
                for (int i = 0; i < hamming_1_sze; ++i) {
                    printf("%s ", article_ids[hamming_1[i]]);
                }
                putchar('\n');
            }
            if (hamming_2_sze) {
                printf("2:");
                for (int i = 0; i < hamming_2_sze; ++i) {
                    printf("%s ", article_ids[hamming_2[i]]);
                }
                putchar('\n');
            }
            if (hamming_3_sze) {
                printf("3:");
                for (int i = 0; i < hamming_3_sze; ++i) {
                    printf("%s ", article_ids[hamming_3[i]]);
                }
                putchar('\n');
            }
        }
        println(sample_ids[sam_idx]);
        if (hamming_0_sze) {
            prints("0:");
            for (int i = 0; i < hamming_0_sze; ++i) {
                prints(article_ids[hamming_0[i]]);
                printc(' ');
            }
            endl();
        }
        if (hamming_1_sze) {
            prints("1:");
            for (int i = 0; i < hamming_1_sze; ++i) {
                prints(article_ids[hamming_1[i]]);
                printc(' ');
            }
            endl();
        }
        if (hamming_2_sze) {
            prints("2:");
            for (int i = 0; i < hamming_2_sze; ++i) {
                prints(article_ids[hamming_2[i]]);
                printc(' ');
            }
            endl();
        }
        if (hamming_3_sze) {
            prints("3:");
            for (int i = 0; i < hamming_3_sze; ++i) {
                prints(article_ids[hamming_3[i]]);
                printc(' ');
            }
            endl();
        }
    }
    flush();
    close_io_handle();
}

int main(int argc, char *argv[]) {
    printf_d("Run!\n");

    // cope with the args
    assert(argc == 3);
    vector_length = str_to_int(argv[1]);
    finger_length = str_to_int(argv[2]);
    // patch_size = (finger_length + 3) / 4;

    read_hash_value(vector_length, finger_length);
    read_stop();

    // ======== read articles ========

    read_whole_articles();

    // sort the feature words (articles)
    walk_trie();
    get_article_features();

#ifdef DEBUG_PRINT_ARTICLE_FEATURE
    FILE *fp = fopen("out.new.txt", "w");
    for (int i = 0; i < article_sze; ++i) {
        fprintf(fp, "[[%s]]\n", article_ids[i]);
        for (int j = 0; j < vector_length; ++j) {
            fprintf(fp, "%d ", word_count[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
#endif

    // work out the articles' fingers
    calculate_finger();

#ifdef DEBUG_PRINT_ARTICLE_FINGER
    for (int i = 0; i < 10; ++i) {
        printf("#%2d: ", i + 1);
        print_u128(article_fingers[i]);
        printf("\n");
    }
#endif

    // ======== read samples ========
    read_whole_samples();
    calculate_finger_sample();

#ifdef DEBUG_PRINT_SAMPLE_INFO
    for (int i = 0; i < sample_sze; ++i) {
        printf("Sample #%2d: %s\n", i + 1, sample_ids[i]);
    }
#endif

    // cmp the fingers
    // I promise that I would make them more graceful!!!
    emit_results();

    return 0;
}