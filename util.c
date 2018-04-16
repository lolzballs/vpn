#include "util.h"

struct shared_ptr_t* shared_ptr_wrap(void *ptr, size_t refs) {
    struct shared_ptr_t *this = malloc(sizeof(struct shared_ptr_t));
    this->ptr = ptr;
    this->refs = refs;
    return this;
}

struct shared_ptr_t* shared_ptr_create(size_t size, size_t refs) {
    void *ptr = malloc(size);
    return shared_ptr_wrap(ptr, refs);
}

void shared_ptr_acquire(struct shared_ptr_t *ptr) {
    ptr->refs++;
}

void shared_ptr_release(struct shared_ptr_t *ptr) {
    if (--ptr->refs == 0) {
        free(ptr->ptr);
        free(ptr);
    }
}

