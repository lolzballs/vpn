#ifndef util_h
#define util_h

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

struct sockaddr;

struct shared_ptr_t {
    void *ptr;
    size_t refs;
};

struct shared_ptr_t* shared_ptr_wrap(void *ptr, size_t refs);
struct shared_ptr_t* shared_ptr_create(size_t size, size_t refs);
void shared_ptr_acquire(struct shared_ptr_t *ptr);
void shared_ptr_release(struct shared_ptr_t *ptr);

int cidr_parse(char *cidr, struct sockaddr *a, uint32_t *mask);
bool sockaddr_cmp(const struct sockaddr *a, const struct sockaddr *b);

#endif

