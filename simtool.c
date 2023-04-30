#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "char_ops.h"
#include "freq_sort.h"
#include "hash_ops.h"
#include "stop_ops.h"
#include "tire.h"

typedef unsigned __int128 finger_t;

#define ARTICLE_CNT 32768
#define SAMPLE_CNT 32768
#define WORD_SZE 4096

int str_to_int(const char *s) {
    int res = 0;
    const char *p = s;
    do {
        res = res * 10 + TODIGIT(*p);
    } while (*++p);
    return res;
}

int vector_length, finger_length;
tire_ndoe_t *total_root;
tire_ndoe_t *sample_root[SAMPLE_CNT];
tire_ndoe_t *article_root[ARTICLE_CNT];

char article_ids[ARTICLE_CNT][64];
int article_sze;
finger_t article_fingers[ARTICLE_CNT];

char sample_ids[SAMPLE_CNT][64];
int sample_sze;
finger_t sample_fingers[ARTICLE_CNT];

int web_weight[10000]; // tmp, each article & sample

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

int tmp;
FILE *stream;

skip_stat_t skip_ret;

int main(int argc, char *argv[]) {
    // cope with the args
    assert(argc == 3);
    vector_length = str_to_int(argv[1]);
    finger_length = str_to_int(argv[2]);

    read_stop_words();
    read_hash_value(vector_length, finger_length);

    // ======== read articles ========
    stream = fopen("article.txt", "r");
    for (;;) {
        // read the article id
        if (fscanf(stream, "%s", article_ids[article_sze++]) == EOF) {
            --article_sze;
            break;
        }
        // read the words in the article
        for (;;) {
            (void) skip(stream);
            fscanf(stream, "%4096[a-zA-Z]", buf);
            skip_ret = skip(stream);
            if (!is_stop_word(buf)) {
                insert_word(&total_root, buf);
                insert_word(&article_root[article_sze - 1], buf);
            }
            if (skip_ret == Eof) {
                goto EndOfReadArticle;
            }
            if (skip_ret == Ff) {
                break;
            }
        }
    }
EndOfReadArticle:
    fclose(stream);

    // ========  ========

    // sort the feature words
    get_sorted_feature(total_root);

    // work out the articles' fingers
    for (int i = 0; i < article_sze; ++i) {
        for (int j = 0; j < vector_length; ++j) {
            web_weight[j] = find_word(article_root[i], freqs[j].word);
        }
        for (int finger_bit = finger_length - 1; finger_bit >= 0; --finger_bit) {
            tmp = 0;
            for (int j = 0; j < vector_length; ++j) {
                tmp += web_weight[j] * ((hashes[j] >> finger_bit & 1) ? 1 : -1);
            }
            article_fingers[i] = article_fingers[i] << 1 | (tmp > 0);
        }
    }

    // ======== read samples ========
    stream = fopen("sample.txt", "r");
    for (;;) {
        // read the sample id
        if (fscanf(stream, "%s", sample_ids[sample_sze++]) == EOF) {
            --sample_sze;
            break;
        }
        // read the words in the article
        for (;;) {
            (void) skip(stream);
            fscanf(stream, "%4096[a-zA-Z]", buf);
            skip_ret = skip(stream);
            if (!is_stop_word(buf)) {
                insert_word(&sample_root[sample_sze - 1], buf);
            }
            if (skip_ret == Eof) {
                goto EndOfReadSample;
            }
            if (skip_ret == Ff) {
                break;
            }
        }
    }
EndOfReadSample:
    fclose(stream);

    // ========  ========

    // figure out the samples' fingers
    for (int i = 0; i < sample_sze; ++i) {
        for (int j = 0; j < vector_length; ++j) {
            web_weight[j] = find_word(sample_root[i], freqs[j].word);
        }
        for (int finger_bit = finger_length - 1; finger_bit >= 0; --finger_bit) {
            tmp = 0;
            for (int j = 0; j < vector_length; ++j) {
                tmp += web_weight[j] * ((hashes[j] >> finger_bit & 1) ? 1 : -1);
            }
            sample_fingers[i] = sample_fingers[i] << 1 | (tmp > 0);
        }
    }

    // cmp the fingers
    // I promise that I would make them more graceful!!!
    stream = fopen("result.txt", "w");
    for (int sam_idx = 0; sam_idx < sample_sze; ++sam_idx) {
        hamming_0_sze = hamming_1_sze = hamming_2_sze = hamming_3_sze = 0;
        for (int art_idx = 0; art_idx < article_sze; ++art_idx) {
            tmp = hamming(article_fingers[art_idx] ^ sample_fingers[sam_idx], finger_length);
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
        if (!sam_idx) {
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
        fputs(sample_ids[sam_idx], stream);
        fputc('\n', stream);
        if (hamming_0_sze) {
            fprintf(stream, "0:");
            for (int i = 0; i < hamming_0_sze; ++i) {
                fprintf(stream, "%s ", article_ids[hamming_0[i]]);
            }
            fputc('\n', stream);
        }
        if (hamming_1_sze) {
            fprintf(stream, "1:");
            for (int i = 0; i < hamming_1_sze; ++i) {
                fprintf(stream, "%s ", article_ids[hamming_1[i]]);
            }
            fputc('\n', stream);
        }
        if (hamming_2_sze) {
            fprintf(stream, "2:");
            for (int i = 0; i < hamming_2_sze; ++i) {
                fprintf(stream, "%s ", article_ids[hamming_2[i]]);
            }
            fputc('\n', stream);
        }
        if (hamming_3_sze) {
            fprintf(stream, "3:");
            for (int i = 0; i < hamming_3_sze; ++i) {
                fprintf(stream, "%s ", article_ids[hamming_3[i]]);
            }
            fputc('\n', stream);
        }
    }
    fclose(stream);

    return 0;
}