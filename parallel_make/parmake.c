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
    if (num_threads < 1) {
        return 0;
    }
    graph_ = parser_parse_makefile(makefile, targets);

    vector *t_vec_of_par = graph_neighbors(graph_, "");
    int c_flag = 0;
    size_t num_targets = vector_size(t_vec_of_par);
    for (size_t j = 0; j < num_targets; ++j) {
        void *curr = vector_get(t_vec_of_par, j);
        if (graph_ == NULL) {
            continue;
        }
        if (!graph_contains_vertex(graph_, curr)) {
            continue;
        }
        vector *k_vec = graph_vertices(graph_);
        size_t k_num = vector_size(k_vec);
        dictionary *d_hist = string_to_int_dictionary_create();

        for (size_t k = 0; k < k_num; ++k) {
            int val_ = 0;
            dictionary_set(d_hist, vector_get(k_vec, k), &val_);
        }
        int buffer = recurr_cycle_num_finder(d_hist, curr);
        dictionary_destroy(d_hist);
        vector_destroy(k_vec);
        
        if (buffer == 1) {
            print_cycle_failure((char*) curr);
            c_flag = 1;
        }
    }



    if (!c_flag) {
        r_vec_ = shallow_vector_create();
        vector *k_vec = graph_vertices(graph_);
        size_t k_num = vector_size(k_vec);
        dictionary *dict_count = string_to_int_dictionary_create();
        for (size_t i = 0; i < k_num; ++i) {
            int val_ = 0;
            dictionary_set(dict_count, vector_get(k_vec, i), &val_);
        }
        rule_finder(r_vec_, t_vec_of_par, dict_count);
        vector_destroy(k_vec);
        dictionary_destroy(dict_count);
        pthread_t threads[num_threads]; 
        for (size_t j = 0; j < num_threads; ++j) {
            // failed to create new process
            if (pthread_create(&threads[j], NULL, routine_, NULL) != 0)
                exit(1);
        }
        for (size_t j = 0; j < num_threads; ++j) {
            // failed to join process
            if (pthread_join(threads[j], NULL) != 0)
                exit(1);
        }
        vector_destroy(r_vec_);
    }
    graph_destroy(graph_);
    vector_destroy(t_vec_of_par);
    return 0;
}



int recurr_cycle_num_finder(dictionary *d_hist, void *t_) {
    int *finder_v = dictionary_get(d_hist, t_);
    if (*finder_v == 2) {
        return 0;
    } else if (*finder_v == 1) {
        return 1;
    } else {
            int *val_ = dictionary_get(d_hist, t_);
        *val_ = 1;
        // check all of its descendants
        vector *neighbors = graph_neighbors(graph_, t_);
        size_t num_neighbors = vector_size(neighbors);
        for (size_t i = 0; i < num_neighbors; ++i) {
            if (recurr_cycle_num_finder(d_hist, vector_get(neighbors, i)) == 1) {
                vector_destroy(neighbors);
                return 1;
            }
        }
        // set val_ to "finished"
        val_ = dictionary_get(d_hist, t_);
        *val_ = 2;
        vector_destroy(neighbors);
        return 0;
    }
    return 0;
}



void rule_finder(vector *result, vector *targets, dictionary *dict_count) {
    size_t num_targets = vector_size(targets);
    for (size_t i = 0; i < num_targets; ++i) {
        void *t_ = vector_get(targets, i);
        vector *t_sub_vec = graph_neighbors(graph_, t_);
        rule_finder(result, t_sub_vec, dict_count);
        if (*((int*)dictionary_get(dict_count, t_)) == 0) {
            int *val = dictionary_get(dict_count, t_);
            *val = 1;
            vector_push_back(result, t_);
        }
        vector_destroy(t_sub_vec);
    }
}


