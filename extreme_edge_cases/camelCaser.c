/**
 * extreme_edge_cases
 * CS 241 - Fall 2021
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char **camel_caser(const char *input_str) {
    // TODO: Implement me!
    if (input_str == NULL) {
        return NULL;
    }
    int sntns_num = 0;
    int chars_num = 0;
    int curr_sntn_num = 0;
    int alpha_pos = 0

    for (int i = 0; i < strlen(input_str); i++) {
        if (ispunct(input_str[i]))) {
            sntns_num++;
        }
    }

    char** sntns = (char**)malloc(sizeof(char*) * (num_sen + 1));
    rslt[sen_num] = NULL;

    for (int j = 0; j < strlen(input_str); j++) {
        if (ispunct(input_str[j])) {
            sntns[curr_sntn_num] = malloc(chars_num + 1);
            memcpy(sntns[curr_sntn_num], input_str + alpha_pos, chars_num);
            alpha_pos = j + 1;
            chars_num = 0;
            curr_sntn_num = curr_sntn_num + 1;
        } else {
            chars_num = chars_num + 1;
        }
    }

    char** rslts = (char**)malloc(sizeof(char*) * (1 + sntns_num));
    for (int k = 0; k < sntns_num; k++) {

        int initial_pos = 0;
        int curr_pos = 0;
        int parse = 0;

        int sntn_lnth = strlen(sntns[k]);

        rslts[k] = malloc(strlen(sntns[k]) + 1);

        char * ptr = rslts[k];
        
        for (int l = 0; l < sntn_lnth; l++) {
            if (isspace(sntns[k][l])) {
                parse = 1;
            
                if (curr_pos) {
                    to_word(sntns[k][l] + initial_pos, curr_pos, ptr);
                    rslts[k] = curr_pos + initial_pos;
                }

                curr_pos = 0;
            } else {
                if (parse) {
                    curr_pos = 1;
                    initial_pos = l;
                    parse = 0
                } else {
                    curr_pos = curr_pos + 1;
                }
            }
        }
        if (curr_pos) {
            to_word(sntns[k] + initial_pos, curr_pos, ptr);
            ptr = ptr + curr_pos;
            curr_pos = 0;
        }

        if (sntn_lnth > 0 && isalpha(ptr[0])) {
            ptr[0] = ptr[0] + 32;
        }

        rslts[k] = ptr;
    }
    destroy(sntns);

    return 
}

void destroy(char **result) {
    // TODO: Implement me!
    for (char** i = result; *i != NULL; i++) {
        free(*i);
    }

    free(result);
}
