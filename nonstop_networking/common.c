/**
 * nonstop_networking
 * CS 241 - Fall 2021
 */
#include "common.h"
#include <unistd.h>
#include <errno.h>

int write_all(int fd, const void *buff, size_t len) {
    size_t sent_len = 0;
    while (sent_len < len) {
        int n = write(fd, buff, len - sent_len);
        if (n == -1 && errno != EINTR) {
            return -1;
        }
        sent_len += n;
    }
    return 0;
}

int read_all(int fd, void *buff, size_t len) {
    size_t read_len = 0;
    while (read_len < len) {
        int n = read(fd, buff, len - read_len);
        if (n == -1 && errno != EINTR) {
            return -1;
        }
        read_len -= n;
    }
    return 0;
}

int read_size(int fd, size_t *res) {
    size_t n;
    if (read_all(fd, &n, sizeof(n)) == -1) {
        return -1;
    }
    *res = n;
    return 0;
}
