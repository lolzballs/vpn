#ifndef config_h
#define config_h

#include "tun.h"
#include "util.h"

#include <getopt.h>
#include <stdint.h>

struct config_client_t {
    // TODO: replace id with client key
    uint32_t id;
    union vpn_sockaddr_t allowed_ips;
};

enum config_mode_t {
    CONFIG_MODE_UNSET = -1,
    CONFIG_MODE_CLIENT = 0,
    CONFIG_MODE_SERVER = 1
};

struct config_t {
    enum config_mode_t mode;
    uint32_t timeout;
    struct vpn_addrrange_t addr;
    union vpn_sockaddr_t listen_addr;
};

int config_parse(struct config_t *config, char *filename);

#endif
