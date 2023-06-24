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

int word_record[TOTAL_WORD + ARTICLE_CNT];
int word_rec_sze;
char article_ids[ARTICLE_CNT][64];
int article_sze;

char sample_ids[SAMPLE_CNT][64];
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

uint_fast32_t count[TRIE_NODE_CNT];
typedef struct _TRIE_NODE {
    int next[27];
} array_trie_node_t;

array_trie_node_t trie_tree[TRIE_NODE_CNT];
uint_fast32_t cur_trie_idx, next_alloc_idx = 1;

void read_stop() {
    int fd = open("stopwords.txt", O_RDONLY);
    struct stat status;
    fstat(fd, &status);
    int passage_len = status.st_size;
    char *buf = mmap(NULL, passage_len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    register char *c = buf;
    while (*c) {
        if (*c == '\r')
            ;
        else if (*c == '\n') {
            count[cur_trie_idx] = 0x80000000u;
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
        count[cur_trie_idx] = 0x80000000u;
        cur_trie_idx = 0;
    }
}

char *passage_buf_map, *sample_buf_map;

void read_whole_articles() {
    int fd = open("article.txt", O_RDONLY);
    struct stat status;
    fstat(fd, &status);
    passage_len = status.st_size;
    passage_buf_map = mmap(NULL, passage_len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    register char *c = passage_buf_map;
    register uint_fast32_t tmp_idx;
    while (*c) {
        while (*c == '\f' || *c == '\r' || *c == '\n') {
            ++c;
        }
        tmp_idx = 0;
        while (*c != '\r' && *c != '\n') {
            article_ids[article_sze][tmp_idx++] = *(c++);
        }
        ++article_sze;
        for (;;) {
            if (ISALPHA(*c)) {
                if (!trie_tree[cur_trie_idx].next[TOINDEX(*c)]) {
                    trie_tree[cur_trie_idx].next[TOINDEX(*c)] = next_alloc_idx++;
                }
                cur_trie_idx = trie_tree[cur_trie_idx].next[TOINDEX(*c)];
                ++c;
            } else {
                ++count[cur_trie_idx];
                word_record[word_rec_sze++] = cur_trie_idx;
                cur_trie_idx = 0;
                while (!ISALPHA(*c)) {
                    if (*c == '\f' || !*c) {
                        word_record[word_rec_sze++] = -1;
                        goto end_of_endless_loop;
                    }
                    ++c;
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
uint_fast32_t stack[TRIE_NODE_CNT];
uint_fast32_t stack_sze;

void walk_trie() {
    uint32_t root;
    stack[stack_sze++] = 0;
    while (stack_sze) {
        root = stack[--stack_sze];
        if (count[root] && !(count[root] >> 31)) {
            mosts[word_sze].idx = root;
            mosts[word_sze].count = count[root];
            ++word_sze;
        }
        for (register int i = 26; i >= 1; --i) {
            if (trie_tree[root].next[i]) {
                stack[stack_sze++] = trie_tree[root].next[i];
            }
        }
    }

    mosts[0].count = 0;
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
    // ==: -1, no swap
    return (((mosts_t *) a)->count >= ((mosts_t *) b)->count) ? -1 : 1;
}

void get_article_features() {
    print_trie(0);
    print_trie_result();
#ifdef DEBUG_PRINT_MOST_FREQS
    for (int i = 0; i < vector_length; ++i) {
        printf("Most freqs: #%3d: %lu\n", i + 1, mosts[i].count);
    }
#endif

    register int word_rec_pos = -1;
    for (int i = 0; i < article_sze; ++i) {
        while (~word_record[++word_rec_pos]) {
            ++count[word_record[word_rec_pos]];
        }
        for (register int j = 0; j < vector_length; ++j) {
            word_count[i][j] = count[mosts[j].idx];
            count[mosts[j].idx] = 0;
        }
    }
}

int *arr_tmp;

// Big data's finger length <= 64

int article_fingers_array[ARTICLE_CNT][64];
int sample_fingers_array[SAMPLE_CNT][64];

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
    int fd = open("sample.txt", O_RDONLY);
    struct stat status;
    fstat(fd, &status);
    passage_len = status.st_size;
    sample_buf_map = mmap(NULL, passage_len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    register char *c = sample_buf_map;
    register uint_fast32_t tmp_idx;
    while (*c) {
        while (*c == '\f' || *c == '\r' || *c == '\n') {
#ifdef DEBUG_PRINT_SAMPLE_INFO
            printf("At begin, read `%c`, sze: %d, next: [%c]\n", *c, sample_sze, *(c + 1));
#endif
            ++c;
        }
        if (passage_len == c - sample_buf_map) {
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
                ++count[cur_trie_idx];
                cur_trie_idx = 0;
                while (!ISALPHA(*c)) {
                    if (*c == '\f' || !*c) {
#ifdef DEBUG_PRINT_SAMPLE_INFO
                        printf("Jump into! Name: %s\n", sample_ids[sample_sze - 1]);
#endif
                        goto end_of_endless_loop_sample;
                    }
                    ++c;
                }
            }
        }
    end_of_endless_loop_sample:
        arr_tmp = word_count[sample_sze - 1];
        for (register int j = 0; j < vector_length; ++j) {
            arr_tmp[j] = count[mosts[j].idx];
            count[mosts[j].idx] = 0;
        }
    }
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

    // the three most frequent start alphabet
    trie_tree[0].next[TOINDEX('s')] = next_alloc_idx++;
    trie_tree[0].next[TOINDEX('c')] = next_alloc_idx++;
    trie_tree[0].next[TOINDEX('p')] = next_alloc_idx++;

    read_hash_value(vector_length, finger_length);
    read_stop();

    // ======== read articles ========

    read_whole_articles();

    walk_trie();
    qsort(mosts, word_sze, sizeof(mosts_t), cmp);
    for (register int j = 0; j < vector_length; ++j) {
        count[mosts[j].idx] = 0;
    }

    int fd_ctp[2];
    assert(pipe(fd_ctp) != -1);

    int pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        // child
        close(fd_ctp[0]);
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
        int patch = (article_sze + 15) / 16;
        for (int i = 0; i < patch; ++i) {
            write(fd_ctp[1], article_fingers_array + i * 16, 4096);
            // printf("Child write: #%d/%d\n", i + 1, patch);
        }
        close(fd_ctp[1]);
        return 0;

    } else {
        // parent
        close(fd_ctp[1]);

        // ======== read samples ========
        read_whole_samples();
        calculate_finger_sample();

#ifdef DEBUG_EMIT_DETAIL_FINGER
        FILE *fps = fopen("finger.sample.new.txt", "w");
        for (int art = 0; art < sample_sze; ++art) {
            fprintf(fps, "[[%s]]\n", sample_ids[art]);
            for (int i = 0; i < finger_length; ++i) {
                fprintf(fps, "%d ", sample_fingers_array[art][i]);
            }
            fputc('\n', fps);
        }
#endif

        int patch = (article_sze + 15) / 16;
        for (int i = 0; i < patch; ++i) {
            read(fd_ctp[0], article_fingers_array + i * 16, 4096);
            // printf("Parent read: #%d/%d\n", i + 1, patch);
        }

#ifdef DEBUG_EMIT_DETAIL_FINGER
        FILE *fpa = fopen("finger.article.new.txt", "w");
        for (int art = 0; art < article_sze; ++art) {
            fprintf(fpa, "[[%s]]\n", article_ids[art]);
            for (int i = 0; i < finger_length; ++i) {
                fprintf(fpa, "%d ", article_fingers_array[art][i]);
            }
            fputc('\n', fpa);
        }
#endif
        close(fd_ctp[0]);

#ifdef DEBUG_PRINT_SAMPLE_INFO
        for (int i = 0; i < sample_sze; ++i) {
            printf("Sample #%2d: %s\n", i + 1, sample_ids[i]);
        }
#endif

        // cmp the fingers
        emit_results();
        return 0;
    }
}