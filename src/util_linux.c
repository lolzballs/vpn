#include "util.h"

#include <string.h>
#include <linux/if.h>
#include <uv.h>

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

int cidr_parse(char *cidr, struct vpn_addrrange_t *range) {
    int res;

    char *mask_str = strtok(cidr, "/");
    range->mask = strtol(mask_str, NULL, 10);
    if (errno != 0) {
        fprintf(stderr, "invalid cidr specified\n");
        return -1;
    }

    if ((res = uv_ip4_addr(cidr, 0, &range->addr.s4)) < 0) {
        fprintf(stderr, "specified ip invalid: %s\n", uv_strerror(res));
        return -1;
    }

    return 0;
}
