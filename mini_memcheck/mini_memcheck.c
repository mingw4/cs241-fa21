/**
 * mini_memcheck
 * CS 241 - Fall 2021
 */
#include "mini_memcheck.h"
#include <stdio.h>
#include <string.h>

int addr_invalid(void*);


size_t total_memory_freed = 0;
size_t total_memory_requested = 0;
size_t invalid_addresses = 0;
meta_data* heaad = NULL;

void *mini_malloc(size_t request_size, const char *filename,
                  void *instruction) {
    // your code here
    if (request_size == 0) {
        return NULL;
    }
    void* mem = malloc(sizeof(meta_data) + request_size);
    if (mem == NULL) {
        return NULL;
    }
    meta_data* mt = (meta_data*) mem;
    void* voke = mem + sizeof(meta_data);
    mt->request_size = request_size;
    mt->filename = filename;
    mt->instruction = instruction;
    mt->next = head;
    head = mt;
    return voke;
}

int addr_invalid(void* payload) {
    if (!payload) {
        return 0;
    }
    if (!head) {
        return 1;
    }
    meta_data* buffer = head;
    while (buffer != NULL) {
        if ((void*) (buffer + 1) == payload) {
            return 0;
        }
        buffer = buffer->next;
    }
    return 1;
}

void *mini_calloc(size_t num_elements, size_t element_size,
                  const char *filename, void *instruction) {
    // your code here
    void* rt = mini_malloc(element_size * num_elements, filename, instruction);
    for (size_t j = 0; j < (element_size * num_elements); j++) {
        *((char*) rt + j) = 0;
    }
    return rt;
}

void *mini_realloc(void *payload, size_t request_size, const char *filename,
                   void *instruction) {
    // your code here

    if (payload == NULL) {
        return mini_malloc(request_size, filename, instruction);
    }
    if (request_size == 0) {
        mini_free(payload);
        return NULL;
    }
    if (addr_invalid(payload)) {
        invalid_addresses = invalid_addresses + 1;
        return NULL;
    }
    meta_data* prev = NULL;
    meta_data* next = NULL;
    meta_data* buffer = head;
    while(buffer != NULL) {
        next = buffer->next;
        void* mem = ((void*)buffer) + sizeof(meta_data);
        if (mem == payload) {
            if (request_size == buffer->request_size) {
                buffer->instruction = instruction;
                buffer->filename = filename;
                return payload;
            }
            if (prev != NULL) {
                prev->next = next;
            } else {
                head = next;
            }
            if (request_size < buffer->request_size) {
                total_memory_freed = total_memory_freed + buffer->request_size - request_size;
            } else {
                total_memory_requested = total_memory_requested + request_size - buffer->request_size;
            }
            void* mem = realloc(buffer, sizeof(meta_data) + request_size);
            if (mem == NULL) {
                return NULL;
            }
            meta_data* mt = (meta_data*) mem;
            void *rt = mem + sizeof(meta_data);
            mt->request_size = request_size;
            mt->instruction = instruction;
            mt->filename = filename;
            mt->next = head;
            head = mt;
            return rt;
        }
        prev = buffer;
        buffer = next;
    }
    return NULL;


}


void mini_free(void *payload) {
    // your code here
    if (payload == NULL) {
        return;
    }
    meta_data* prev = NULL;
    meta_data* next = NULL;
    meta_data* buffer = head;

    while (buffer != NULL) {
        next = buffer->next;
        void* mem = (void*) buffer + sizeof(meta_data);
        if (payload == mem) {
            if (prev == NULL) {
                head = next;
            } else {
                prev->next = next;
            }
            total_memory_freed = total_memory_freed + buffer->request_size;
            free(buffer);
            return;
        }
        prev = buffer;
        buffer = next;
    }
    
    invalid_addresses = invalid_addresses + 1;
}
