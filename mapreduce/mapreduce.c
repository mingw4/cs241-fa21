/**
 * mapreduce
 * CS 241 - Fall 2021
 */
#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>


int main(int argc, char **argv) {
    if (argc != 6) {
        print_usage();
        return 1;
    }
    char *in_f = argv[1];
    char *out_f = argv[2];
    char *map = argv[3];
    char *redc = argv[4];
    char *num_str = argv[5];
    size_t num = 0;
    sscanf(num_str, "%zu", &num);
    int* pm[num];
    for (size_t j = 0; j < num; j++) {
        pm[j] = calloc(2, sizeof(int));
        pipe(pm[j]);
    }
    int pr[2];
    pipe(pr);
    int po = 1;
    int ln = strcmp("stdout", out_f);
    if (0 == ln) {
        po = 1;
    } else {
        FILE *file_ = fopen(out_f, "w");
        po = fileno(file_);
    }
    pid_t pids[num];
    for (size_t j = 0; j < num; j++) {
        pid_t pid_ = fork();
        pids[j] = pid_;
        if (pid_ == 0) {
            char buffer[10];
            sprintf(buffer, "%zu", j);
            close(pm[j][0]);
            dup2(pm[j][1], 1);
            execlp("./splitter", "./splitter", in_f, argv[5]);
            exit(1);
        }
    }
    pid_t pidss[num];
    for (size_t j = 0; j < num; j++) {
        pid_t pid_ = fork();
        pidss[j] = pid_;
        if (pid_ == 0) {
            close(pm[j][0]);
            dup2(pm[j][0], 0);
            dup2(pr[1], 1);
            execl(map, map, NULL);
            exit(1);
        }
    }
    close(pr[1]);
    pid_t pid_ = fork();
    if (pid_ != 0) {
        close(pr[0]);
        close(po);
    } else {
        dup2(pr[0], 0);
        dup2(po, 1);
        execl(redc, redc, NULL);
        exit(1);
        close(pr[0]);
        close(po);
    }
    int flag_ = 0;
    for (size_t j = 0; j < num; j++) {
        waitpid(pids[j], &flag_, 0);
    }
    for (size_t j = 0; j < num; j++) {
        close(pm[j][0]);
        waitpid(pidss[j], &flag_, 0);
    }
    waitpid(pid_, &flag_, 0);
    if (flag_ == 1) {
        print_nonzero_exit_status(redc, flag_);
    }
    print_num_lines(out_f);
    for (size_t j = 0; j < num; j++) {
        free(pm[j]);
    }

    // Create an input pipe for each mapper.
    

    // Create one input pipe for the reducer.

    // Open the output file.


    // Start a splitter process for each mapper.


    // Start all the mapper processes.

    // Start the reducer process.

    // Wait for the reducer to finish.

    // Print nonzero subprocess exit codes.

    // Count the number of lines in the output file.

    return 0;
}
