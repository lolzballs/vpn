#ifndef ipv4_h
#define ipv4_h

#include <stdint.h>

struct ipv4hdr {
    uint8_t version: 4;
    uint8_t ihl: 4;
    uint8_t dcsp: 6;
    uint8_t ecn: 2;
    uint16_t len;
    uint16_t iden;
    uint16_t flags: 3;
    uint16_t fragoff: 13;
    uint8_t ttl;
    uint8_t proto;
    uint16_t chksum;
    uint32_t saddr;
    uint32_t daddr;

    uint8_t payload[];
};

#endif

