
  
/**
 * deadlock_demolition
 * CS 241 - Fall 2021
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>

struct drm_t {pthread_mutex_t x;};

pthread_mutex_t ini_ = PTHREAD_MUTEX_INITIALIZER;

//hepler func.
int cycleExists(pthread_t *thread_id);


graph *graph_ = NULL;
set *iterated_ = NULL;

drm_t *drm_init() {
    /* Your code here */
    drm_t *result = malloc(sizeof(drm_t));
    pthread_mutex_init(&result->x, NULL);
    pthread_mutex_lock(&ini_);
    if (graph_ == NULL) {
        graph_ = shallow_graph_create();
    }
    graph_add_vertex(graph_, result);
    pthread_mutex_unlock(&ini_);
    return result;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    pthread_mutex_lock(&ini_);
    if ((graph_contains_vertex(graph_, drm) == false) || (graph_contains_vertex(graph_, thread_id) == false)) {
        pthread_mutex_unlock(&ini_);
        return 0;
    }
    if (graph_adjacent(graph_, drm, thread_id) == true) {
        graph_remove_edge(graph_, drm, thread_id);
        pthread_mutex_unlock(&drm->x);
    }
    pthread_mutex_unlock(&ini_);
    return 1;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
        pthread_mutex_lock(&ini_);
        graph_add_vertex(graph_, thread_id);
    if (graph_adjacent(graph_, drm, thread_id) == true) {
        pthread_mutex_unlock(&ini_);
        return 0;
    }
    graph_add_edge(graph_, thread_id, drm);
    if (cycleExists(thread_id) == 0) {
        pthread_mutex_unlock(&ini_);
        pthread_mutex_lock(&drm->x);
        pthread_mutex_lock(&ini_);
        graph_remove_edge(graph_, thread_id, drm);
        graph_add_edge(graph_, drm, thread_id);
        pthread_mutex_unlock(&ini_);
        return 1;
    } else {
        graph_remove_edge(graph_, thread_id, drm);
        pthread_mutex_unlock(&ini_);
        return 0;
    }
    return 0;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    graph_remove_vertex(graph_, drm);
    pthread_mutex_destroy(&drm->x);
    free(drm);
    pthread_mutex_destroy(&ini_);
    return;
}

int cycleExists(pthread_t *thread_id) {
    if (iterated_ == NULL) {
        iterated_ = shallow_set_create();
    }
    if (set_contains(iterated_, thread_id) == false) {
        set_add(iterated_, thread_id);
        vector *adjacents = graph_neighbors(graph_, thread_id);
        for (size_t j = 0; j < vector_size(adjacents); j++) {
            if (cycleExists(vector_get(adjacents, j)) == 1) {
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