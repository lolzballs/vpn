#include "tun.h"

#include <getopt.h>
#include <stdint.h>

enum config_mode_t {
    CONFIG_MODE_UNSET = -1,
    CONFIG_MODE_CLIENT = 0,
    CONFIG_MODE_SERVER = 1
};

struct config_t {
    enum config_mode_t mode;
    uint32_t timeout;
    char ip[64];
    struct sockaddr_in addr;
};

static struct option arguments[] = {
    { "timeout", required_argument, NULL, 't'},
    { 0 }
};


