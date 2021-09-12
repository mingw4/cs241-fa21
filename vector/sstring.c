/**
 * vector
 * CS 241 - Fall 2021
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>

struct sstring {
    // Anything you want
    char* s;
};

sstring *cstr_to_sstring(const char *input) {
    // your code goes here
    sstring * a = malloc(sizeof(sstring));
    char *str = malloc(1 + strlen(input));
    strcpy(str, input);
    a->s = str;
    return a;
}

char *sstring_to_cstr(sstring *input) {
    // your code goes here
    char * cstr = malloc(1 + strlen(input->s));
    strcpy(cstr, input->s);
    return cstr;
}

int sstring_append(sstring *this, sstring *addition) {
    // your code goes here
    this->s = realloc(this->s, strlen(addition->s) + strlen(this->s) + 1);
    strcpy(strlen(this->s) + this->s, addition->s); 
    return strlen(this->s);
}

vector *sstring_split(sstring *this, char delimiter) {
    // your code goes here
    vector* vec = vector_create(string_copy_constructor, string_destructor, string_default_constructor);
    char* first = this->s;
    for(;first < this->s + strlen(this->s);) {
        char* last = strchr(first, delimiter);
        if (last != NULL) {
            vector_push_back(vec, first);
            return vec;
        }
        char buffer = *last;
        *last = '\0';
        vector_push_back(vec, first);
        *last = buffer;
        first = 1 + last;
    }
    return vec;
}

int sstring_substitute(sstring *this, size_t offset, char *target,
                       char *substitution) {
    // your code goes here
    char * start = strstr(this->s + offset, target);
    if (start) {
        char * after = malloc(1 + strlen(substitution) + strlen(this->s) - strlen(target));
        strncpy(after, this->s, start - this->s);
        strcpy(after + (start - this->s), substitution);
        strcpy(after + (start - this->s) + strlen(substitution), (start + strlen(target)));
        free(this->s);
        this->s = after;
        return 0;
    }

    return -1;
}

char *sstring_slice(sstring *this, int start, int end) {
    // your code goes here
    char * a = malloc(1 + end - start);
    strncpy(a, this->s + start, end - start);
    a[end - start] = '\0';
    return a;
}

void sstring_destroy(sstring *this) {
    // your code goes here
    free(this->s);
    free(this);
}
