/**
 * critical_concurrency
 * CS 241 - Fall 2021
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    /* Your code here */
    queue *result = malloc(sizeof(queue));
    if (result == NULL) {
        return NULL;
    }
    result->head = NULL;
    result->tail = NULL;
    result->size = 0;
    result->max_size = max_size;
    pthread_cond_init(&(result->cv), NULL);
    pthread_mutex_init(&(result->m), NULL);
    return result;
}

void queue_destroy(queue *this) {
    /* Your code here */
    if (!this) {
        return;
    }
    queue_node* curr = this->head;
    while(curr) {
        queue_node* buffer = curr;
        curr = curr->next;
        free(buffer);
    }
    pthread_cond_destroy(&(this->cv));
    pthread_mutex_destroy(&(this->m));
    free(this);
}

void queue_push(queue *this, void *data) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    while (this->max_size > 0 && this->size >= this->max_size) {
        pthread_cond_wait(&(this->cv), &(this->m));
    }
    queue_node* result = malloc(sizeof(queue_node));
    result->data = data;
    result->next = NULL;
    if (this->size != 0) {
        this->tail->next = result;
        this->tail = result;
        this->size = this->size + 1;
        pthread_cond_broadcast(&(this->cv));
        pthread_mutex_unlock(&(this->m));
    } else {
        this->head = result;
        this->tail = result;
        this->size = this->size + 1;
        pthread_cond_broadcast(&(this->cv));
        pthread_mutex_unlock(&(this->m));
    }
}

void *queue_pull(queue *this) {
    /* Your code here */
    pthread_mutex_lock(&(this->m));
    while (this->head == NULL) {
        pthread_cond_wait(&(this->cv), &(this->m));
    }
    queue_node *buffer = this->head;
    if (this->size == 1) {
        this->head = NULL;
        this->tail = NULL;
    } else {
        this->head = buffer->next;
    }
    void* result = buffer->data;
    this->size = this->size - 1;
    free(buffer);
    pthread_cond_broadcast(&(this->cv));
    pthread_mutex_unlock(&(this->m));

    return result;
}
