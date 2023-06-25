#include "optimize.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// #define DEBUG

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "array.h"
#include "char_ops.h"
#include "consts.h"
#include "hash_ops.h"
#include "patch_io.h"

#ifdef DEBUG
#define printf_d printf
#else
#define printf_d(...) ;
#endif

typedef uint64_t finger_t;

int str_to_int(const char *__restrict s) {
    int res = 0;
    const char *p = s;
    do {
        res = res * 10 + TODIGIT(*p);
    } while (*++p);
    return res;
}

int vector_length, finger_length;

array_t word_rec_array;
int word_rec_default = -1;
// int word_record[TOTAL_WORD + ARTICLE_CNT];
// int word_rec_sze;
char article_ids[ARTICLE_CNT][130];
int article_sze;

char sample_ids[SAMPLE_CNT][130];
int sample_sze;

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

// uint_fast32_t count[300000];
array_t count_arr;
uint_fast32_t count_default = 0;
typedef struct _TRIE_NODE {
    int next[27];
} array_trie_node_t;

array_trie_node_t zero_trie_node;

// array_trie_node_t trie_tree[TRIE_NODE_CNT];
array_t trie_arr;
uint_fast32_t cur_trie_idx;

void read_stop() {
    FILE *fp = fopen("stopwords.txt", "rb");

    char buf[1024];
    while (fscanf(fp, "%s", buf) != EOF) {
        // printf("[%s]\n", buf);
        size_t len = strlen(buf);
        cur_trie_idx = 0;
        for (int i = 0; i < len; ++i) {
            // printf("cur: %zd, size: %zd, cap: %zd\n", cur_trie_idx, trie_arr.size, trie_arr.capacity);
            if (!((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(buf[i])]) {
                ((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(buf[i])] =
                    append_array(&trie_arr, &zero_trie_node);
                append_array(&count_arr, &count_default);
            }
            cur_trie_idx = ((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(buf[i])];
        }
        ((uint_fast32_t *) count_arr.dat)[cur_trie_idx] = 0x80000000u;
    }

    fclose(fp);
}

char *passage_buf_map, *sample_buf_map;

void read_whole_articles() {
    cur_trie_idx = 0;
    FILE *fp = fopen("article.txt", "rb");

    // register char *c = passage_buf_map;
    int c = fgetc(fp);
    register uint_fast32_t tmp_idx;
    while (c != EOF && c != 0) {
        while (c == '\f' || c == '\r' || c == '\n') {
            c = fgetc(fp);
        }
        tmp_idx = 0;
        if (c == EOF) {
            break;
        }
        while (c != '\r' && c != '\n') {
            article_ids[article_sze][tmp_idx++] = c;
            c = fgetc(fp);
        }
        ++article_sze;
        for (;;) {
            if (ISALPHA(c)) {
                if (!((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(c)]) {
                    ((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(c)] =
                        append_array(&trie_arr, &zero_trie_node);
                    append_array(&count_arr, &count_default);
                }
                cur_trie_idx = ((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(c)];
                c = fgetc(fp);
            } else {
                ++((uint_fast32_t *) count_arr.dat)[cur_trie_idx];
                append_array(&word_rec_array, &cur_trie_idx);
                cur_trie_idx = 0;
                while (!ISALPHA(c)) {
                    if (c == '\f' || c == 0 || c == EOF) {
                        append_array(&word_rec_array, &word_rec_default);
                        goto end_of_endless_loop;
                    }
                    c = fgetc(fp);
                }
            }
        }
    end_of_endless_loop:;
    }
    fclose(fp);
}

typedef struct {
    uint_fast32_t idx, count;
} mosts_t;
// mosts_t mosts[WORD_CNT];
array_t most_arr;

// uint_fast32_t word_sze;
uint_fast32_t stack[1000000];
uint_fast32_t stack_sze;

void walk_trie() {
    uint32_t root;
    stack[stack_sze++] = 0;
    while (stack_sze) {
        root = stack[--stack_sze];
        if (((uint_fast32_t *) count_arr.dat)[root] && !(((uint_fast32_t *) count_arr.dat)[root] >> 31)) {
            // mosts[word_sze].idx = root;
            // mosts[word_sze].count = count[root];
            append_array(&most_arr, &(mosts_t){.idx = root, .count = ((uint_fast32_t *) count_arr.dat)[root]});
            // ++word_sze;
        }
        for (register int i = 26; i >= 1; --i) {
            if (((array_trie_node_t *) trie_arr.dat)[root].next[i]) {
                stack[stack_sze++] = ((array_trie_node_t *) trie_arr.dat)[root].next[i];
            }
        }
    }

    ((mosts_t *) most_arr.dat)[0].count = 0;
}

int cmp(const void *a, const void *b) {
    // ==: -1, no swap
    return (((mosts_t *) a)->count >= ((mosts_t *) b)->count) ? -1 : 1;
}

void get_article_features() {
    qsort(((mosts_t *) most_arr.dat), size_array(&most_arr), sizeof(mosts_t), cmp);
    for (register int j = 0; j < vector_length; ++j) {
        ((uint_fast32_t *) count_arr.dat)[((mosts_t *) most_arr.dat)[j].idx] = 0;
    }
    register int word_rec_pos = -1;
    for (int i = 0; i < article_sze; ++i) {
        while (~((int *) word_rec_array.dat)[++word_rec_pos]) {
            ++((uint_fast32_t *) count_arr.dat)[((int *) word_rec_array.dat)[word_rec_pos]];
        }
        for (register int j = 0; j < vector_length; ++j) {
            word_count[i][j] = ((uint_fast32_t *) count_arr.dat)[((mosts_t *) most_arr.dat)[j].idx];
            ((uint_fast32_t *) count_arr.dat)[((mosts_t *) most_arr.dat)[j].idx] = 0;
        }
    }
}

int *arr_tmp;

int article_fingers_array[ARTICLE_CNT][130];
int sample_fingers_array[SAMPLE_CNT][130];

void calculate_finger() {
    for (int art = 0; art < article_sze; ++art) {
        web_weight = word_count[art];
        for (int i = 0; i < vector_length; ++i) {
            if (!web_weight[i]) {
                continue;
            }
            for (int j = 0; j < finger_length; ++j) {
                article_fingers_array[art][j] += hash[i][j] * web_weight[i];
            }
        }
        for (int j = 0; j < finger_length; ++j) {
            article_fingers_array[art][j] = (unsigned) article_fingers_array[art][j] >> 31;
        }
    }
}

void read_whole_samples() {
    FILE *fp = fopen("sample.txt", "rb");

    int c = fgetc(fp);
    register uint_fast32_t tmp_idx;
    while (c != EOF) {
        while (c == '\f' || c == '\r' || c == '\n') {
            c = fgetc(fp);
        }
        if (c == EOF) {
            break;
        }
        tmp_idx = 0;
        while (c != '\r' && c != '\n') {
            sample_ids[sample_sze][tmp_idx++] = c;
            c = fgetc(fp);
        }
        ++sample_sze;

        for (;;) {
            if (ISALPHA(c)) {
                if (!((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(c)]) {
                    ((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(c)] =
                        append_array(&count_arr, &count_default);
                    append_array(&trie_arr, &(array_trie_node_t){.next = {0}});
                }
                cur_trie_idx = ((array_trie_node_t *) trie_arr.dat)[cur_trie_idx].next[TOINDEX(c)];
                c = fgetc(fp);
            } else {
                ++((uint_fast32_t *) count_arr.dat)[cur_trie_idx];
                cur_trie_idx = 0;
                while (!ISALPHA(c)) {
                    if (c == '\f' || !c || c == EOF) {
                        goto end_of_endless_loop_sample;
                    }
                    c = fgetc(fp);
                }
            }
        }
    end_of_endless_loop_sample:
        arr_tmp = word_count[sample_sze - 1];
        for (register int j = 0; j < vector_length; ++j) {
            arr_tmp[j] = ((uint_fast32_t *) count_arr.dat)[((mosts_t *) most_arr.dat)[j].idx];
            ((uint_fast32_t *) count_arr.dat)[((mosts_t *) most_arr.dat)[j].idx] = 0;
        }
    }

    fclose(fp);
}

void calculate_finger_sample() {
    for (int art = 0; art < sample_sze; ++art) {
        web_weight = word_count[art];
        for (int i = 0; i < vector_length; ++i) {
            if (!web_weight[i]) {
                continue;
            }
            for (int j = 0; j < finger_length; ++j) {
                sample_fingers_array[art][j] += hash[i][j] * web_weight[i];
            }
        }
        for (int j = 0; j < finger_length; ++j) {
            sample_fingers_array[art][j] = sample_fingers_array[art][j] < 0;
        }
    }
}

void emit_results() {
    open_write_handle("result.txt");
    int art_idx, j;
    for (int sam_idx = 0; sam_idx < sample_sze; ++sam_idx) {
        hamming_0_sze = hamming_1_sze = hamming_2_sze = hamming_3_sze = 0;
        for (art_idx = 0; art_idx < article_sze; ++art_idx) {
            tmp = 0;
            for (j = 0; j < finger_length; ++j) {
                tmp += article_fingers_array[art_idx][j] ^ sample_fingers_array[sam_idx][j];
                if (tmp > 3) {
                    goto HugeDiffPruning;
                }
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
        HugeDiffPruning:;
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

    // Init arraies
    count_arr = init_array(sizeof(uint_fast32_t), TRIE_NODE_CNT);
    count_arr.size = 1;
    trie_arr = init_array(sizeof(array_trie_node_t), TRIE_NODE_CNT);
    trie_arr.size = 1;
    word_rec_array = init_array(sizeof(int), TOTAL_WORD + ARTICLE_CNT);
    most_arr = init_array(sizeof(mosts_t), WORD_CNT);

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
    FILE *fps = fopen("finger.sample.new.txt", "w");
    for (int art = 0; art < sample_sze; ++art) {
        fprintf(fps, "[[%s]]\n", sample_ids[art]);
        for (int i = 0; i < finger_length; ++i) {
            fprintf(fps, "%d ", sample_fingers_array[art][i]);
        }
        fputc('\n', fps);
    }
#endif

    // cmp the fingers
    emit_results();

    return 0;
}