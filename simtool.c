#include "patch_io.h"
#include <string.h>

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

#include "char_ops.h"
#include "consts.h"
#include "freq_sort.h"
#include "hash_ops.h"
#include "mempool.h"
#include "patch_io.h"
#include "stop_ops.h"
#include "trie.h"

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
trie_ndoe_article_t *total_root;
trie_ndoe_t *sample_root[SAMPLE_CNT];

char article_ids[ARTICLE_CNT][64];
char *article_starts[ARTICLE_CNT], *article_ends[ARTICLE_CNT];
int article_sze;
finger_t article_fingers[ARTICLE_CNT];

char sample_ids[SAMPLE_CNT][64];
int sample_sze;
finger_t sample_fingers[ARTICLE_CNT];

// int web_weight[10000]; // tmp, each article & sample
int *web_weight;

// the distance for each sample
// I promise that I would make them more graceful!!!
int hamming_0[ARTICLE_CNT];
int hamming_0_sze;
int hamming_1[ARTICLE_CNT];
int hamming_1_sze;
int hamming_2[ARTICLE_CNT];
int hamming_2_sze;
int hamming_3[ARTICLE_CNT];
int hamming_3_sze;

char buf[WORD_SZE];
int buf_sze;

int word_count[ARTICLE_CNT][10000] /*__attribute__((__aligned__(32)))*/;

unsigned int tmp;
unsigned __int128 tmp128;
FILE *stream;

int skip_ret;

// __m256i avx_a, avx_b, avx_c;
// int32_t avx_arr[8] __attribute__((__aligned__(32)));

void read_stop_and_hash() {
    printf_d("Here\n");
    // new_pool(sze_node, &pool);
    new_pool(sze_article, &pool);
    new_pool(sze_article_count, &pool_article);

    new_article(-1, &total_root);
    read_stop_words();
    printf_d("Rear stop words over!\n");

    read_hash_value(vector_length, finger_length);
    printf_d("Rear hashes over!\n");

    printf_d("Before new_pool\n");

    printf_d("start to read articles\n");
}

// ==== new version start ====
char article_buf[PATCH_BUF_SZE];
uint_fast32_t article_len;

typedef struct _TRIE_NODE {
    uint_fast32_t count;
    int next[27];
} array_trie_node_t;

array_trie_node_t trie_tree[50000000];
uint_fast32_t cur_trie_idx, next_alloc_idx = 1;

void read_stop() {
    FILE *fp = fopen("./stopwords.txt", "rb");
    article_buf[fread_unlocked(article_buf, 1, PATCH_BUF_SZE, fp)] = '\n';
    fclose(fp);
    register char *c = article_buf;
    while (*c) {
        if (*c == '\r')
            ;
        else if (*c == '\n') {
            trie_tree[cur_trie_idx].count = UINT32_MAX;
            cur_trie_idx = 0;
        } else {
            if (!trie_tree[cur_trie_idx].next[TOINDEX(*c)]) {
                trie_tree[cur_trie_idx].next[TOINDEX(*c)] = next_alloc_idx++;
            }
            cur_trie_idx = trie_tree[cur_trie_idx].next[TOINDEX(*c)];
        }
    }
}

void read_whole_articles() {
    FILE *fp = fopen("./article.txt", "rb");
    article_len = fread_unlocked(article_buf, 1, PATCH_BUF_SZE, fp);
    fclose(fp);
    register char *c = article_buf;
    register uint_fast32_t tmp_idx;
    while (*c) {
        tmp_idx = 0;
        while (*c != '\r' && *c != '\n') {
            article_ids[article_sze][tmp_idx] = *(c++);
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
                if (~trie_tree[cur_trie_idx].count >> 31) {
                    ++trie_tree[cur_trie_idx].count;
                    cur_trie_idx = 0;
                }
                while (!ISALPHA(*c)) {
                    if (*c == '\f') {
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
mosts_t mosts[1048576];
uint_fast32_t word_sze;
uint_fast32_t stack[1048576];
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
        if (trie_tree[root].count > 0) {
            mosts[word_sze].idx = root;
            mosts[word_sze].count = trie_tree[root].count;
        }
        for (register int i = 26; i >= 1; --i) {
            if (trie_tree[root].next[i]) {
                stack[stack_sze++] = trie_tree[root].next[i];
            }
        }
    }
}

int cmp(const void *a, const void *b) {
    // return memcmp(&(((mosts_t *) a)->count), &(((mosts_t *) b)->count), sizeof(uint_fast32_t));
    return ((mosts_t *) a)->count > ((mosts_t *) b)->count ? 1 : -1; // ==: -1, no swap
}

// uint_fast32_t count_1[10005], count_2[10005];
// uint_fast32_t *count_ptr[] = {count_1, count_2};
// uint_fast8_t count_cur = 1, count_last = 0;
// uint_fast32_t *cptr_cur, *cptr_last;

void get_article_features() {
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
// ==== new version end ====

void read_articles() {
    open_read_handle("article.txt");
    for (;;) {
        // read the article id
        if (ask_string(article_ids[article_sze++]) == EOF) {
            --article_sze;
            break;
        }
        printf_d("article: %05d\n", article_sze);
        // read the words in the article
        new_article(article_sze - 1, &total_root);
        for (;;) {
            (void) skip_noalpha();
            printf_d("<read word>\n");
            // ask_word(buf);
            read_word();
            printf_d("</read word>\n");
            // printf("In main loop: Tag 02: %s\n", buf);
            skip_ret = skip_noalpha();
            // if (!is_stop_word(buf)) {
            //     printf_d("<Read %s>\n", buf);
            //     insert_article_word(&total_root, buf, article_sze - 1);
            //     printf_d("</Read>\n");
            // }
            if (skip_ret == -1) {
                // EOF
                goto EndOfReadArticle;
            }
            if (skip_ret == 1) {
                // '\f'
                break;
            }
        }
    }
EndOfReadArticle:
    close_io_handle();

    printf_d("read articles over!\n");
}

int *arr_tmp;

void get_features() {
    // int head = 0, tail = 0;
    // while (head < vector_length) {
    //     while (/*printf("%d => %d\n", tail, freqs[idx[tail]]->count), */ -1 == freqs[idx[tail]]->count) {
    //         ++tail;
    //     }
    //     idx[head++] = idx[tail++];
    // }
    for (int j = 0; j < vector_length; ++j) {
        arr_tmp = freqs[idx[j]]->article_cnt;
        for (int i = 0; i < article_sze; ++i) {
            word_count[i][j] = arr_tmp[i];
        }
        // memcpy(word_count[j], arr_tmp, sizeof(int) * article_sze);
    }
    // for (int j = 0; j < vector_length; ++j) {
    //     printf("%d\n", word_count[0][j]);
    // }
}

void get_features_sample() {
    for (int j = 0; j < vector_length; ++j) {
        arr_tmp = freqs[idx[j]]->article_cnt;
        for (int i = 0; i < sample_sze; ++i) {
            word_count[i][j] = arr_tmp[i + article_sze];
        }
    }
}

void unused_foo_3_1(int i, int j) {
    web_weight[j] = freqs[idx[j]]->article_cnt[i];
}

void calculate_finger() {
    for (int i = 0; i < article_sze; ++i) {
        // for (int j = 0; j < vector_length; ++j) {
        //     // foo_3_1(i, j);
        // }
        web_weight = word_count[i];
        tmp128 = 0;
        for (int finger_bit = 0; finger_bit < finger_length; ++finger_bit) {
            tmp = 0x80000000u;
            arr_tmp = hash[finger_bit];
            // ptrdiff_t diff = web_weight - arr_tmp;
            // for (int *j = arr_tmp; *j; ++j) {
            //     tmp += *(j + diff) * *j;
            // }
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

void read_samples() {
    open_read_handle("sample.txt");
    for (;;) {
        // read the sample id
        if (ask_string(sample_ids[sample_sze++]) == EOF) {
            --sample_sze;
            break;
        }
        // read the words in the article
        new_article(article_sze + sample_sze - 1, &total_root);
        for (;;) {
            (void) skip_noalpha();
            // ask_word(buf);
            read_word();
            skip_ret = skip_noalpha();
            // if (!is_stop_word(buf)) {
            //     insert_article_word(&total_root, buf, article_sze + sample_sze - 1);
            // }
            if (skip_ret == -1) {
                // EOF
                goto EndOfReadSample;
            }
            if (skip_ret == 1) {
                // '\f'
                break;
            }
        }
    }
EndOfReadSample:
    close_io_handle();
}

void calculate_finger_sample() {
    for (int i = 0; i < sample_sze; ++i) {
        // for (int j = 0; j < vector_length; ++j) {
        //     web_weight[j] = freqs[idx[j]]->article_cnt[i + article_sze];
        // }
        web_weight = word_count[i];
        tmp128 = 0;
        for (int finger_bit = 0; finger_bit < finger_length; ++finger_bit) {
            arr_tmp = hash[finger_bit];
            tmp = 0x80000000u;
            // ptrdiff_t diff = web_weight - arr_tmp;
            // for (int *j = arr_tmp; *j; ++j) {
            //     tmp += *(j + diff) * *j;
            // }
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
            // tmp = hamming(article_fingers[art_idx] ^ sample_fingers[sam_idx], finger_length);
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
    patch_size = (finger_length + 3) / 4;

    read_stop_and_hash();

    // ======== read articles ========

    read_articles();

    // ========  ========

    // sort the feature words
    get_sorted_feature_article(total_root);

    printf_d("Sorted  Over!\n");

    get_features();

    // work out the articles' fingers
    calculate_finger();
    // ======== read samples ========
    // new_pool(sze_node);
    read_samples();

    // ========  ========
    get_features_sample();
    // figure out the samples' fingers
    calculate_finger_sample();

    // cmp the fingers
    // I promise that I would make them more graceful!!!
    emit_results();

    return 0;
}