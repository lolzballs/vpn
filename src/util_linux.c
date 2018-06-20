#include "util.h"

#include <linux/if.h>

bool sockaddr_cmp(const struct sockaddr *a, const struct sockaddr *b) {
    if (a->sa_family != b->sa_family) {
        return false;
    }
    for (ssize_t j = 0; j < 14; j++) {
        if (a->sa_data[j] != b->sa_data[j]) {
            return false;
        }
    }
    return true;
}
