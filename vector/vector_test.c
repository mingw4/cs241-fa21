/**
 * vector
 * CS 241 - Fall 2021
 */
#include "vector.h"
#include <stdio.h>
int main(int argc, char *argv[]) {
    // Write your test cases here
    vector * v = vector_create(unsigned_int_copy_constructor, unsigned_int_destructor, unsigned_int_default_constructor);
    int e0 = 0;
    int e1 = 1;
    int e2 = 2;
    int e3 = 3;
    int e4 = 4;
    int e5 = 5;
    int e6 = 6;
    int e7 = 7;
    int e8 = 8;
    int e9 = 9;
    int e10 = 10;
    vector_push_back(v, &e0);
    vector_push_back(v, &e1);
    vector_push_back(v, &e2);
    vector_push_back(v, &e3);
    vector_push_back(v, &e4);
    vector_push_back(v, &e5);
    vector_push_back(v, &e6);
    vector_push_back(v, &e7);
    vector_push_back(v, &e8);
    vector_push_back(v, &e9);
    vector_push_back(v, &e10);

    printf("v is empty: %d\n", vector_empty(v));
    printf("v size is: %zu\n", vector_size(v));
    printf("first element of v is: %d\n", **((int**)vector_begin(v)));

    vector_destroy(v);


    return 0;
}
