/**
 * extreme_edge_cases
 * CS 241 - Fall 2021
 */
#include "camelCaser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>


char **camel_caser(const char * input_str) {
    if (input_str == NULL) {
        return NULL;
    }
    char ** result = NULL;
    char * sen = NULL;
    unsigned long idx_char = 0;
    unsigned long num_sen = 0;
    unsigned long len = strlen(input_str);
    for (unsigned long j = 0; j < len; j++) {
        if (ispunct(input_str[j])) {
            num_sen = num_sen + 1;
        }
    }

    result = malloc(sizeof(char*) * (num_sen + 1));
    result[num_sen] = NULL;

    unsigned long itr_sen = 0;

    for (unsigned long k = 0; k < len; k++) {
        if (ispunct(input_str[k])) {
            sen = malloc(sizeof(char*) * (1 + idx_char));
            result[itr_sen] = sen; 
            result[itr_sen][idx_char] = '\0';
            itr_sen = itr_sen + 1;
            idx_char = 0;

        } else if (isspace(input_str[k])) {
            continue;
        } else {
            idx_char = idx_char + 1;
        }

    }
    char buffer;
    num_sen = 0;
    itr_sen = 0;
    idx_char = 0;
    bool flag_upp = false;
    bool flag_first = false;
    for (unsigned long l = 0; l < len; l++) {
        if (!result[num_sen]) {
            break;
        } else if (ispunct(input_str[l])) {
            num_sen = num_sen + 1;
            flag_upp = false;
            flag_first = true;
            idx_char = 0;
        } else if (isspace(input_str[l])) {
            flag_upp = true;
        } else {
            if (isalpha(input_str[l])) {
                if (flag_upp && !flag_first) {
                    buffer = toupper(input_str[l]);
                } else {
                    buffer = tolower(input_str[l]);
                }
            } else {
                buffer = input_str[l];
            }
            result[num_sen][idx_char++] = buffer;
            flag_upp = false;
            flag_first = false;
        }

    }
    return result;

}

void destroy(char **result) {
    // TODO: Implement me!
    for (char** i = result; (*i); i++) {
        free(*i);
    }

    free(result);
}
