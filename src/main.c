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
    struct config_t config = {
        .mode = CONFIG_MODE_UNSET,
        .timeout = 10,
    };

    if (argc != 2) {
        goto usage;
    }

    if (config_parse(&config, argv[1]) == -1) {
        goto usage;
    }

    return vpn_start(config);
    
usage:
    printf("usage: %s <config>\n", argv[0]);
    return -1;
}

