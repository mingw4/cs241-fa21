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
        for (int n = 1; argv[n]; n++) {
            if (strcmp("--", argv[n])) {

                char *k = strtok(argv[n], "=");
                char *v = strtok(NULL, "");
                if (v == NULL) {
                    print_env_usage();
                }
                char *itr = k;
                while (*itr) {
                    if (*itr != '_' && (!isdigit(*itr)) && (!isalpha(*itr))) {
                        print_env_usage();
                    }
                    itr++;
                }
                if (v[0] == '%') {
                    v = getenv(v + 1);
                    if (!v) {
                        print_environment_change_failed();
                    }
                } else {
                    char *itr = v;
                    while (*itr) {
                        if (*itr != '_' && (!isdigit(*itr)) && (!isalpha(*itr))) {
                            print_env_usage();
                        }
                        itr++;
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
    } else if (p < 0) {
        print_fork_failed();
    } else {
        int flag_;
        waitpid(p, &flag_, 0);
    }
    return 0;
}