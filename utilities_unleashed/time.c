/**
 * utilities_unleashed
 * CS 241 - Fall 2021
 */
#include <unistd.h>
#include "format.h"
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_time_usage();
    }
    pid_t p = fork();
    if (p == 0) {
        execvp(argv[1], (argv + 1));
        print_exec_failed();
    } else if (p < 0) {
        print_fork_failed();
    } else {
        int flag_;
        double length_;
        struct timespec initial;
        struct timespec termination;
        clock_gettime(CLOCK_MONOTONIC, &initial);
        pid_t final = waitpid(p, &flag_, 0);
        clock_gettime(CLOCK_MONOTONIC, &termination);
        if (WIFEXITED(flag_) && final != -1) {
            length_ = (0.0 - initial.tv_nsec + termination.tv_nsec) / (0.0 + 1000000000L) + (0.0 - initial.tv_sec + termination.tv_sec);
            display_results(argv, length_);
        }
    }


    return 0;

}
