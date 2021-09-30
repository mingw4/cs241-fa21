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
void* start_routine(void* p) {
    structure_t* rt = (structure_t*) p;
    for (size_t j = 0; j < rt->list_len; j++) {
        rt->base_case = rt->reduce_func(rt->base_case, rt->list[j]);
    }
    pthread_exit(NULL);
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    if (num_threads >= list_len) {
        return reduce(list, list_len, reduce_func, base_case);
    }
    pthread_t pthreads[num_threads];
    int thread_size = list_len / num_threads;
    structure_t* par = calloc(num_threads, sizeof(structure_t));
    for (size_t j = 0; j < num_threads - 1; j++) {
        par[j].list = list + j * thread_size;
        par[j].list_len = thread_size;
        par[j].reduce_func = reduce_func;
        par[j].base_case = base_case;
        pthread_create(pthreads + j, NULL, start_routine, par + j);
    }
    par[num_threads - 1].list = list + ((num_threads - 1) * thread_size);
    par[num_threads - 1].list_len = list_len - ((num_threads - 1) * thread_size);
    par[num_threads - 1].reduce_func = reduce_func;
    par[num_threads - 1].base_case = base_case;
    pthread_create(pthreads + (num_threads - 1), NULL, start_routine, par + (num_threads - 1));
    for (size_t k = 0; k < num_threads; k++) {
        pthread_join(pthreads[k], NULL);
    }
    int result = base_case;
    for (size_t l = 0; l < num_threads; l++) {
        result = reduce_func(result, par[l].base_case);
    }
    return result;
 
}
