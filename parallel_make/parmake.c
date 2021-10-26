/**
 * parallel_make
 * CS 241 - Fall 2021
 */

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "includes/queue.h"
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "parser.h"
#include "includes/vector.h"
#include "includes/graph.h"
#include <sys/types.h>
#include "includes/set.h"
#include <stdio.h>
#include <sys/stat.h>

pthread_cond_t p_cond_;
pthread_mutex_t p_mutex_;

set* iterated_ = NULL;
graph* graph_ = NULL;
queue* queue_ = NULL;
bool flag_;

void* start_routine_(void* addr) {
    while (1) {
        rule_t* r = queue_pull(queue_);
        if (!r) {
            queue_push(queue_, NULL);
            break;
        }
        for (size_t j = 0; j < vector_size(r->commands); j++) {
            if (system(vector_get(r->commands, j)) != 0) {
                flag_ = true;
                break;
            }
        }
        r->state = 1;
        pthread_cond_signal(&p_cond_);
    }
    return NULL;
}

int recur_cycle_detector(char *target) {
    if (iterated_ == NULL) {
        iterated_ = shallow_set_create();
    }
    if (set_contains(iterated_, target) == false) {
        set_add(iterated_, target);
        vector *adjacents = graph_neighbors(graph_, target);
        for (size_t j = 0; j < vector_size(adjacents); j++) {
            if (recur_cycle_detector(vector_get(adjacents, j)) == 1) {
                return 1;
            }
        }
    } else {
        iterated_ = NULL;
        return 1;
    }
    iterated_ = NULL;
    return 0;
}

int is_to_unlock (vector* vec) {
    for (size_t j = 0; j < vector_size(vec); j++) {
        if (!((rule_t *) graph_get_vertex_value(graph_, vector_get(vec, j)))->state) {
            return 0;
        }
    }
    return 1;
}

int recur_failure_detector (char* aim) {
    bool is_accessable = false;
    if (0 > access(aim, F_OK)) {
        is_accessable = true;
    }
    vector *adjacents = graph_neighbors(graph_, aim);
    for (size_t j = 0; j < vector_size(adjacents); j++) {
        char * adjacent = vector_get(adjacents, j);
        if (0 > access(adjacent, F_OK) && is_accessable == false) {
            is_accessable = true;
        } else if (access(adjacent, F_OK) == 0 && access(aim, F_OK) == 0 && is_accessable == false) {
            struct stat aim_;
            stat(aim, &aim_);
            struct stat adjacent_;
            stat(adjacent, &adjacent_);
            if (0 > difftime(aim_.st_mtime, adjacent_.st_mtime)) {
                is_accessable = true;
            }
        }
        if (!(((rule_t *) graph_get_vertex_value(graph_, adjacent))->state) && recur_failure_detector(adjacent)) {
            return 1;
        }
    }
    if (vector_size(adjacents)) {
        pthread_mutex_lock(&p_mutex_);
        while (!is_to_unlock(adjacents)) {
            pthread_cond_wait(&p_cond_, &p_mutex_);
        }
        pthread_mutex_unlock(&p_mutex_);
    }
    vector_destroy(adjacents);
    rule_t* rule_ = (rule_t *) graph_get_vertex_value(graph_, aim);
    if (!(rule_->state) && is_accessable) {
        queue_push(queue_, rule_);
    } else {
        rule_->state = 1;
    }
    return 0;
}

int parmake(char *makefile, size_t num_threads, char **targets) {
    // good luck!
    queue_ = queue_create(-1);
    pthread_t pthreads_[num_threads];
    for (size_t j = 0; j < num_threads; j++) {
        pthread_create(pthreads_ + j, NULL, start_routine_, NULL);
    }
    pthread_cond_init(&p_cond_, NULL);
    pthread_mutex_init(&p_mutex_, NULL);
    graph_ = parser_parse_makefile(makefile, targets);
    vector *aims_ = graph_neighbors(graph_, "");
    for (size_t j = 0; j < vector_size(aims_); j++) {
        if (recur_cycle_detector(vector_get(aims_, j)) == 0) {
            if (recur_failure_detector(vector_get(aims_, j))) {
                flag_ = false;
            }
        } else {
            print_cycle_failure(vector_get(aims_, j));
        }
    }
    queue_push(queue_, NULL);
    for (size_t k = 0; k < num_threads; k++) {
        pthread_join(pthreads_[k], NULL);
    }
    pthread_mutex_destroy(&p_mutex_);
    pthread_cond_destroy(&p_cond_);
    graph_destroy(graph_);
    vector_destroy(aims_);
    set_destroy(iterated_);
    return 0;
}
