/**
 * parallel_make
 * CS 241 - Fall 2021
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "includes/dictionary.h"
#include "includes/graph.h"
#include "includes/vector.h"
#include "includes/queue.h"

//helper functions
void *routine_(void*);
int recurr_cycle_num_finder(dictionary*, void*);
int recurr_cycle_num_finder(dictionary*, void*);

//helper structures
graph *graph_ = NULL;
vector *r_vec_ = NULL;

//pthread stuff
pthread_cond_t g_cond_ = PTHREAD_COND_INITIALIZER;
pthread_cond_t r_cond_ = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_lock_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t r_lock_ = PTHREAD_MUTEX_INITIALIZER;

//helper flags
int start_flag_(void*);



int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!


    return 0;
}