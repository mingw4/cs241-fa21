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
    char arr[] = {0, 0, 0, 0, 0, 15, 0};
    strange_step(arr);

    return 0;
}