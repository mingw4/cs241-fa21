/**
 * teaching_threads
 * CS 241 - Fall 2021
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */

typedef struct structure_t {
    int *list;
    size_t list_len;
    reducer reduce_func;
    int base_case;
} structure_t;

/* You should create a start routine for your threads. */
void* start_routine(void* par) {
    structure_t *buffer = (structure_t*) par;
    int *res = malloc(sizeof(int));
    *res = reduce(buffer->list, buffer->list_len, buffer->reduce_func, buffer->base_case);
    return (void*) res;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    if (num_threads >= list_len) {
        return reduce(list, list_len, reduce_func, base_case);
    }
    int lists[num_threads];
    pthread_t pthreads[num_threads];
    int thread_size = list_len / num_threads;
    structure_t* par[num_threads];
    for (size_t j = 0; j < num_threads - 1; j++) {
        par[j] = malloc(sizeof(structure_t));
        par[j]->list = list + j * thread_size;
        par[j]->list_len = thread_size;
        par[j]->reduce_func = reduce_func;
        par[j]->base_case = base_case;
    }
    par[num_threads] = malloc(sizeof(structure_t));
    par[num_threads]->list = list + num_threads * thread_size;
    par[num_threads]->list_len = list_len - (thread_size * num_threads);
    par[num_threads]->reduce_func = reduce_func;
    par[num_threads]->base_case = base_case;

    for (size_t k = 0; k < num_threads; k++) {
        pthread_create(&pthreads[k], 0, start_routine, (void*)par[k]);
    }
    for (size_t l = 0; l < num_threads; l++) {
        void *buffer;
        pthread_join(pthreads[l],&buffer);
        lists[l] = *((int*)buffer);
        free(buffer);
        free(par[l]);
    }
    int result = reduce(lists, num_threads, reduce_func, base_case);
    return result;
}
