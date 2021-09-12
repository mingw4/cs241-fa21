/**
 * vector
 * CS 241 - Fall 2021
 */
#include "sstring.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    // TODO create some tests
    sstring* test = cstr_to_sstring("abcd");
    char* sliced = sstring_slice(test, 1, 3);
    printf("sliced is: %s\n", sliced);
    free(sliced);
    sstring_destroy(test);
    return 0;
}
