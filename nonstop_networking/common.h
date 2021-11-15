/**
 * nonstop_networking
 * CS 241 - Fall 2021
 */
#pragma once
#include <stddef.h>
#include <sys/types.h>

#define LOG(...)                      \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n");        \
    } while (0);

#define MAX_FILENAME_LEN 255
#define MAX_VERB_LEN 6

typedef enum { GET, PUT, DELETE, LIST, V_UNKNOWN } verb;

int write_all(int fd, const void *buff, size_t len);
int read_all(int fd, void *buff, size_t len);
int read_size(int fd, size_t *res);
