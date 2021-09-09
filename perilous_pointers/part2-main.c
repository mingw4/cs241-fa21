/**
 * perilous_pointers
 * CS 241 - Fall 2021
 */
#include "part2-functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * (Edit this function to print out the "Illinois" lines in
 * part2-functions.c in order.)
 */
int main() {
    // your code here
    first_step(81);
    int a = 132;
    second_step(&a);
    int b = 8942;
    int *c = &b;
    int **d = &c;
    double_step(d);
    char e[] = {1, 2, 3, 4, 5, 15, 0, 0, 0, 0};
    strange_step(e);

    char* f = "abc";
    empty_step((void*) f);

    char* g = "uuuuuuuuuu";
    two_step((void*) g, g);
    three_step(g, g + 2, g + 4);
    char* h = "cccsk";
    step_step_step(h, h + 2, h);

    
    return 0;
}