/**
 * shell
 * CS 241 - Fall 2021
 */
#include "format.h"
#include "shell.h"
#include "vector.h"
#include <stdio.h>
#include "sstring.h"
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <getopt.h>

void sgn_(int sig);
int Fg (char * cmd);
void destroy_();
void Open_source(int argc, char *argv[]);
void Exit_source();
void logical_command(vector * a ,char * b);

typedef struct process {
    char *command;
    pid_t pid;
} process;

static vector *Present;
static vector *hst = NULL;
static char *hst_in = NULL;
static FILE *source = NULL;



void sgn_(int sig) {
    if (SIGCHLD == sig) {
        pid_t p;
        while (p = waitpid(-1, 0, WNOHANG) > 0) {
            size_t input_length = vector_size(Present);
            for (unsigned long j = 0; j < input_length; j++) {
                process *tmp = (process*) vector_get(Present, j);
                if (tmp->pid == p) {
                    free(tmp->command);
                    free(tmp);
                    vector_erase(Present, j);
                    return;
                }
            }
        }
    }
}

void destroy_() {
    size_t l = vector_size(Present);
    for (unsigned long j = 0; j < l; j++) {
        process *tmp = (process *) vector_get(Present, j);
        kill(tmp->pid, SIGKILL);
        free(tmp->command);
        free(tmp);
    }
    vector_destroy(Present);
}

void Open_source(int argc, char *argv[]) {
    source = stdin;
    int p;
    while ((p = getopt(argc, argv, "h:f:")) != -1) {

        if (p == 'f') {
            FILE *r = fopen(optarg, "r");
            if (!r) {
                print_script_source_error();
                exit(1);
            }
            source = r;
        } else if (p == 'h') {
            FILE *w = fopen(optarg, "r");
            if (!w) {
                print_hst_source_error();
                exit(1);
            }
            char *b = NULL;
            size_t b_size = 0;

            while (getline(&b, &b_size, w) != -1) {
                if (b[strlen(b) - 1] == '\n') b[strlen(b) - 1] = '\0';
                vector_push_back(hst, (void *)b);
            }
            free(b);
            fclose(w);
            w = NULL;
            hst_in = get_full_path(optarg);
        }  
    }
}

void Exit_source() {
    if (source != stdin) {
        fclose(source);
    }
    if (hst_in != NULL) {
        FILE *hst_source = fopen(hst_in, "w");
        for (size_t j = 0; j < vector_size(hst); ++j) {
            fprintf(hst_source, "%s\n", (char*) vector_get(hst, j));
        }
        fclose(hst_source);
    }
    vector_destroy(hst);
}

int Fg(char *cmd) {
    if (!strncmp(cmd,"cd",2)) {
        cmd = cmd + 3;
        if (chdir(cmd) < 0) {
            print_no_directory(cmd);
            return 1;
        } else {
            return 0;
        }
    } else {
        fflush(stdout);
        int bg = 0;
        if (cmd[strlen(cmd) - 1] == '&') {
            bg = 1;
        }
        pid_t f_ = fork();
        if (f_ > 0) {
            process *tmp = calloc(1, sizeof(Present));
            tmp->command = calloc(strlen(cmd) + 1, sizeof(char));
            strcpy(tmp->command, cmd);
            tmp->pid = f_;

            vector_push_back(Present, tmp);
            if (bg == 1) {
                if (setpgid(f_, f_) == -1) {
                    print_setpgid_failed();
                    exit(1);
                }
            } else {
                if (setpgid(f_, getpid()) == -1) {
                    print_setpgid_failed();
                    exit(1);
                }
                int status;
                int error = waitpid(f_, &status, 0);
                if (error != -1) {
                    if (WIFEXITED(status) && WEXITSTATUS(status)) {
                        return 1;
                    }
                } else {
                    print_wait_failed();
                    exit(1);
                }
            }
        }
        if (f_ == 0) {
            if (bg == 1) {
                cmd[strlen (cmd) - 1] = '\0';
            }
            vector *aa = sstring_split(cstr_to_sstring(cmd), ' ');
            char *tet[vector_size(aa)+1];
            for (size_t i = 0; i < vector_size(aa); i++) {
                tet[i] = (char *) vector_get(aa, i);
            }
            if (!strcmp(tet[vector_size(aa)-1], ""))
                tet[vector_size(aa)-1] = NULL;
            else
                tet[vector_size(aa)] = NULL;
            print_cmd_executed(getpid());
            execvp(tet[0], tet);
            print_exec_failed(tet[0]);
            exit(1);
        } else {
            print_fork_failed();
            exit(1);
        }


    }
    return 0;
}



int shell(int argc, char *argv[]) {
    signal(SIGINT, sgn_);
    signal(SIGCHLD, sgn_);


    source = stdin;
    hst = string_vector_create();
    Present = shallow_vector_create();
    Open_source(argc, argv);


    char *in = NULL;
    size_t bsize = 0;
    ssize_t b;
    while (1) {

        char *full_path = get_full_path("./");
        print_prompt(full_path, getpid());
        free(full_path);

        b = getline(&in,&bsize, source);

        if (b > 0 && in[b - 1] == '\n') {
            in[b - 1] = '\0';
            if (source != stdin) print_cmd(in);
        }

        if (-1 == b) {
        destroy_();
        break;
        }

        size_t hst_size = vector_size(hst);

        if (!strcmp(in, "!hst")) {
            for (unsigned long j = 0; j < hst_size; j++) {
                print_hst_line(j, (char *) vector_get(hst, j));
            }
        } else if (in[0] == '!') {
            for (int itr = hst_size - 1; itr >= 0 ; itr--) {
                char *cmd = (char *)vector_get(hst, itr);

                if (in[1] == '\0' || !strncmp(in + 1, cmd, strlen(in + 1))) {
                    print_cmd(cmd); vector_push_back(hst, cmd); Fg(cmd);
                    break;
                }

                while (itr == 0) {
                    print_no_hst_match();
                    break;
                }
            }
        } else if ('#' == in[0]) {
            size_t num;
            size_t in_num = sscanf(in+1, "%zu", &num);

            if (!in_num || hst_size - 1 < num) {
                print_invalid_index();
            } else {
                char *cmd = (char *)vector_get(hst, num);
                print_cmd(cmd); vector_push_back(hst, cmd); Fg(cmd);
            }
        } else if (!strcmp(in,"exit")) {
            destroy_(); 
            break;
        } else {
            vector_push_inack(hst, in);
            int flag_ = 0;
            sstring *sstring_ = cstr_to_sstring(in);
            vector *pt = sstring_split(sstring_, ' ');
            for (unsigned long j = 0; j < vector_size(pt); j++) {
                if (!strcmp(vector_get(pt, j), "&&")) {
                    flag_++;
                    char *p = strtok(in, "&");
                    p[strlen(p) - 1] = '\0';
                    char *q = strtok(NULL, "");
                    q += 2;
                    if(!Fg(p)) {
                        Fg(q);
                    }
                }
                if (!strcmp(vector_get(pt, j), "||")) {
                    flag_++;
                    char *p = strtok(in, "|");
                    p[strlen(p)-1] = '\0';
                    char *q = strtok(NULL, "");
                    q+=2;
                    if(Fg(p)) {
                        Fg(q);
                    }
                }
                if (((char *) vector_get(pt, j))[strlen(vector_get(pt, j))-1] == ';') {
                    flag_++;
                    char *p = strtok(in, ";");
                    char *q = strtok(NULL, "");
                    q++;
                    Fg(p); {
                        Fg(q);
                    }
                }
            }
            if (!flag_) {
                Fg(in);
            }
            vector_destroin(pt);
            sstring_destroin(sstring_);
        }
    }

    free(in);

    Exit_source();

    return 0;
}
