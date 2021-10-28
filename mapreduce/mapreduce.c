
#include "utils.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//Partner: mingw4, shunl2, zhichao8, qz13


int main(int argc, char **argv) {
    // init
    if (argc != 6) exit(1);
    int mapper_count = atoi(argv[5]);
    pid_t splitter_pid[mapper_count];
    pid_t mapper_pid[mapper_count];

    // Create an input pipe for each mapper.
    int mapper_pipe[mapper_count * 2];
    for (int i = 0; i < mapper_count; i++) {
      pipe(&mapper_pipe[i * 2]);
    }

    // Create one input pipe for the reducer.
    int reducer_pipe[1 * 2];
    pipe(reducer_pipe);

    // Open the output file.
    int output_file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);

    // Start a splitter process for each mapper.
    for (int i = 0; i < mapper_count; i++) {
        splitter_pid[i] = fork();
        if (!splitter_pid[i]) {
            close(mapper_pipe[i * 2]);
            dup2(mapper_pipe[i * 2 + 1], 1);
            char temp_str[10];
            sprintf(temp_str, "%d", i);
            execl("./splitter", "./splitter", argv[1], argv[5], temp_str, NULL);
            exit(1);
        } else if (splitter_pid[i] == -1) {
            exit(1);
        }
    }

    // Start all the mapper processes.
    for (int i = 0; i < mapper_count; i++) {
        close(mapper_pipe[i * 2 + 1]);
        mapper_pid[i] = fork();
        if (!mapper_pid[i]) {
            close(reducer_pipe[0]);
            dup2(mapper_pipe[i * 2], 0);
            dup2(reducer_pipe[1], 1);
            execl(argv[3], argv[3], NULL);
            exit(1);
        } else if (mapper_pid[i] == -1) {
            exit(1);
        }
    }

    // Start the reducer process.
    close(reducer_pipe[1]);
    pid_t reducer_pid = fork();
    if (!reducer_pid) {
        dup2(reducer_pipe[0], 0);
        dup2(output_file, 1);
        execl(argv[4], argv[4], NULL);
        exit(1);
    } else if (reducer_pid == -1) {
        exit(1);
    }
    close(reducer_pipe[0]);
    close(output_file);

    // Wait for the reducer to finish.
    for (int i = 0; i < mapper_count; i++) {
      int status;
      waitpid(splitter_pid[i], &status, 0);
    }
    for (int i = 0; i < mapper_count; i++) {
      close(mapper_pipe[i * 2]);
      int status;
      waitpid(mapper_pid[i], &status, 0);
    }
    
    // Print nonzero subprocess exit codes.
    int status;
    waitpid(reducer_pid, &status, 0);
    if (status) {
        print_nonzero_exit_status(argv[4], status);
    }

    // Count the number of lines in the output file.
    print_num_lines(argv[2]);
    return 0;
}