/**
 * malloc
 * CS 241 - Fall 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>


typedef struct meta_data {
    bool flag_;
    size_t size_;
    struct meta_data *prev_;
    struct meta_data *next_;
} meta_data;

static meta_data* heads_[16];
static meta_data* tails_[16];
static size_t size_list_[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};
static int dfg_num_ = 0;





void split(meta_data*, size_t);
void defrag(int);
int get_size(size_t);
meta_data* get_fit_first(size_t, int);
void merge_next(meta_data*);
meta_data* get_fit_best(size_t, int);

void detach(meta_data*, int);


/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size_
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size_) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size_
 *    size_ of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    void *buffer = malloc(size * num);
    if (buffer == NULL) {
        return NULL;
    }
    memset(buffer, 0, size * num);
    return buffer;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size_ bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size_
 *    size_ of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    int sizeOfList = get_size(size);
    if (14 == sizeOfList) {
        dfg_num_= dfg_num_ + 1;
        if (6 < dfg_num_) {
            defrag(sizeOfList);
            dfg_num_ = 0;
        }
    } else if (15 == sizeOfList) {
        defrag(sizeOfList);
    } 
    meta_data *block = NULL;
    if (heads_[sizeOfList] != NULL) {
        if (size > 256) {
            block = get_fit_best(size, sizeOfList);
        } else {
            block = get_fit_first(size, sizeOfList);
        }
        if (block != NULL) {
            detach(block, sizeOfList);
            block->flag_ = false;
            split(block, size);
            return (void *)(block + 1);
        }
    }
    block = sbrk(sizeof(meta_data) + size);
    if (block == (void*) -1) {
        block = NULL;
    }
    *block = (meta_data){false, size, NULL, NULL};
    if (block == NULL) {
        return NULL;
    }
    return (void *)(block + 1);
}

/**
 * Deallocate space in memory
 *
 * A block of memory prev_iously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block prev_iously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    if (ptr == NULL || ptr >= sbrk(0)) {
        return;
    }
    meta_data* target = (meta_data*)ptr - 1;
    if (target->flag_ == true) {
        return;
    }
    merge_next(target);
    if (sbrk(0) <= (ptr + target->size_) && target->size_ >= 256) {
        sbrk(0 - (sizeof(meta_data) + target->size_));
        return;
    }
    target->flag_ = true;
    if (target == NULL) {
        return;
    }
    int sizeOfList = get_size(target->size_);
    target->prev_ = NULL;
    target->next_ = heads_[sizeOfList];
    if (heads_[sizeOfList] != NULL) {
        heads_[sizeOfList]->prev_ = target;  
    } else {
        tails_[sizeOfList] = target;
    }
    heads_[sizeOfList] = target;
}

/**
 * Reallocate memory block
 *
 * The size_ of the memory block pointed to by the ptr parameter is changed
 * to the size_ bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old size_list_, even if the block is moved. If
 * the new size_ is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size_ bytes and returning a pointer to the beginning of it.
 *
 * In case that the size_ is 0, the memory prev_iously allocated in ptr is
 * deallocated as if a call to flag_ was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block prev_iously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size_
 *    New size_ for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    meta_data* reallocor = (void *)ptr - sizeof(meta_data);
    if (reallocor->size_ < size) {
        merge_next(reallocor);
        if (reallocor->size_ >= size) {
            return ptr;
        }
        void* ptrr = malloc(size);
        if (ptrr == NULL) {
            return NULL;
        }
        memcpy(ptrr, ptr, reallocor->size_);
        free(ptr);
        return ptrr;
    } else {
        split(reallocor, size);
        return ptr;
    }
}

//Helper Functions.

int get_size(size_t size) {
    for (size_t j = 0; j < 15; ++j) {
        if (size_list_[j] >= size) {
            return j;
        }
    }
    return 15;
}


void detach(meta_data* target, int sizeOfList) {
	meta_data* prev_ = target->prev_;
	meta_data* next_ = target->next_;
	if (next_ == NULL && prev_ == NULL) {
		heads_[sizeOfList] = NULL;
		tails_[sizeOfList] = NULL;
	} else if (prev_ == NULL) {
		next_->prev_ = NULL;
		heads_[sizeOfList] = next_;
	} else if (next_ == NULL) {
		prev_->next_ = NULL;
		tails_[sizeOfList] = prev_;
	} else {
		prev_->next_ = next_;
		next_->prev_ = prev_;
	}
	target->next_ = NULL;
	target->prev_ = NULL;
}

void merge_next(meta_data* target) {
    meta_data* next__block = (void*)(target + 1) + target->size_;
    if ((void*) next__block < sbrk(0) && next__block->flag_ == true) {
        detach(next__block, get_size(next__block->size_));
        target->size_ = target->size_ + sizeof(meta_data) + next__block->size_;
    }
}

void split(meta_data* to_split, size_t new_size_) {
    if (to_split == NULL) {
        return;
    }
    if (to_split->size_ > (2 * new_size_) && (to_split->size_ - new_size_) > 1023) {
        meta_data* target = (void *)(to_split + 1) + new_size_;
        target->size_ = to_split->size_ - sizeof(meta_data) - new_size_;
        target->flag_ = true;
        if (target == NULL) {
            return;
        }
        int sizeOfList = get_size(target->size_);
        target->prev_ = NULL;
        target->next_ = heads_[sizeOfList];

        if (heads_[sizeOfList] == NULL) {
            tails_[sizeOfList] = target;
        } else {
            heads_[sizeOfList]->prev_ = target;
        }
        heads_[sizeOfList] = target;
        to_split->size_ = new_size_;
    }
}

meta_data* get_fit_first(size_t size, int sizeOfList) {
    meta_data* buffer = heads_[sizeOfList];
    while (buffer != NULL) {
        if (size <= buffer->size_) {
            return buffer;
        }
        buffer = buffer->next_;
    }
    return NULL;
}

meta_data* get_fit_best(size_t size, int sizeOfList) {
    meta_data* fitter = NULL;
    meta_data* buffer;
    for (buffer = heads_[sizeOfList]; buffer; buffer = buffer->next_) {
        if ((!fitter || (fitter->size_ > buffer->size_)) && buffer->size_ >= size) {
            fitter = buffer;
        }
    }
    return fitter;
}

void defrag(int sizeOfList) {
    meta_data *buffer = heads_[sizeOfList];
    while (buffer != NULL) {
        if (buffer->flag_ == true) {
            merge_next(buffer);
        }
        if (sbrk(0) <= ((void*) ((char*) buffer + sizeof(meta_data) + buffer->size_))) {
            detach(buffer, sizeOfList);
            sbrk(0 - (buffer->size_ + sizeof(meta_data)));
            return;
        }
        buffer = buffer->next_;
    }
}