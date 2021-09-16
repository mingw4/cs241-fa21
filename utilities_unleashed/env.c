/**
 * utilities_unleashed
 * CS 241 - Fall 2021
 */
#include "format.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

int main(int argc, char *argv[]) {

    if (argc < 4) {
        print_env_usage();
    }
    pid_t p = fork();
    if (p == 0) {
        for (unsigned int n = 1; argv[n]; n++) {
            if (strcmp("--", argv[n])) {
                char* v = strtok(NULL, "");
                char* k = strtok(argv[n], "=");
                if (!v) {
                    print_env_usage();
                }
                for (char* itr = k; (*itr); itr++) {
                    if (!(isalpha(*itr) || isdigit(*itr) || *itr == '_')) {
                        print_env_usage();
                    }
                }
                if (v[0] != '%') {
                    for (char* itr = v; (*itr); itr++) {
                        if (!(isalpha(*itr) || isdigit(*itr) || *itr == '_')) {
                            print_environment_change_failed();
                        }
                    }
                } else {
                    v = getenv(v + 1);
                    if (!v) {
                        print_environment_change_failed();
                    }
                }
                if (setenv(k, v, 1) < 0) {
                    print_environment_change_failed();
                }
            } else {
                execvp(argv[n + 1], argv + n + 1);
                print_exec_failed();
            }
        }
        print_env_usage();
    } else if (p > 0) {
        int flag_;
        waitpid(p, &flag_, 0);
        return 0;
    } else {
        print_fork_failed();
    }


    return 0;
}
