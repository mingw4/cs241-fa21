/**
 * critical_concurrency
 * CS 241 - Fall 2021
 */
#include "barrier.h"

// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
    int error = 0;
    pthread_cond_destroy(&(barrier->cv));
    pthread_mutex_destroy(&(barrier->mtx));
    return error;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
    int error = 0;
    barrier->n_threads = num_threads;
    barrier->count = 0;
    barrier->times_used = 1;
    pthread_cond_init(&(barrier->cv), NULL);
    pthread_mutex_init(&(barrier->mtx), NULL);
    return error;
}

int barrier_wait(barrier_t *barrier) {
    pthread_mutex_lock(&(barrier->mtx));

    while (barrier->times_used != 1) {
        pthread_cond_wait(&(barrier->cv), &(barrier->mtx));
    }

    barrier->count = barrier->count + 1;

    if (barrier->n_threads != barrier->count) {
        while (barrier->times_used == 1 && barrier->n_threads != barrier->count) {
            pthread_cond_wait(&(barrier->cv), &(barrier->mtx));
        }
        barrier->count = barrier->count - 1;
        if (barrier->count == 0) {
            barrier->times_used = 1;
        }
    } else {
        barrier->times_used = 0;
        barrier-> count = barrier->count - 1;
        pthread_cond_broadcast(&(barrier->cv));
    }
    pthread_mutex_unlock(&(barrier->mtx));
    return 0;
}
