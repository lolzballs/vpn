#include "packethdr.h"
#include "tun.h"
#include "util.h"
#include "trie.h"
#include "vpn.h"

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

    // TODO: evaluate safety of this
    while ((ch = getopt_long(argc, argv, "cst:", arguments, NULL)) != -1) {
        switch (ch) {
            case 's':
                config.mode = CONFIG_MODE_SERVER;
                break;
            case 'c':
                config.mode = CONFIG_MODE_CLIENT;
                break;
            case 't':
                config.timeout = strtol(optarg, NULL, 10);
                if (errno != 0) {
                    goto usage;
                }
                break;
            default:
                goto usage;
        }
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

    config.addr = server_addr;
    strncpy(config.ip, argv[ipind], 64);

    return vpn_start(config);
    
usage:
    printf("usage: %s [-t timeout] {-s|-c} <tun_ip> <ip> <port>\n", argv[0]);
    return -1;
}

