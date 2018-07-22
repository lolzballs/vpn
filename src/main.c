#include "packethdr.h"
#include "tun.h"
#include "util.h"
#include "trie.h"
#include "vpn.h"
#include "config.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

int main(int argc, char *argv[]) {
    int res, ipind;
    char ch;
    struct sockaddr_in server_addr;
    struct config_t config = {
        .mode = CONFIG_MODE_UNSET,
        .timeout = 10,
    };

    if (config_parse(&config, "vpn.conf") == -1) {
        goto usage;
    }

    ipind = optind;

    if (config.mode == CONFIG_MODE_UNSET)
        goto usage;

    if (argc != ipind + 3)
        goto usage;

    if ((res = uv_ip4_addr(argv[ipind + 1], atoi(argv[ipind + 2]), &server_addr)) < 0) {
        fprintf(stderr, "specified ip invalid: %s\n", uv_strerror(res));
        goto usage;
    }

    return vpn_start(config);
    
usage:
    printf("usage: %s [-t timeout] {-s|-c} <tun_ip> <ip> <port>\n", argv[0]);
    return -1;
}

