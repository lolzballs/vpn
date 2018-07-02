#ifndef util_h
#define util_h

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>

union vpn_sockaddr {
    struct sockaddr sa;
    struct sockaddr_in s4;
    struct sockaddr_in6 s6;
    struct sockaddr_storage ss;
};

struct vpn_addrrange {
    union vpn_sockaddr addr;
    uint64_t mask;
};

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

static inline int min(int a, int b) {
    if (a > b)
        return b;
    return a;
}
static inline int max(int a, int b) {
    if (a > b)
        return a;
    return b;
}

#endif

