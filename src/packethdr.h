#ifndef packethdr_h
#define packethdr_h

#include <stdint.h>

#define ETH_P_IPv4 0x0008
#define ETH_P_IPv6 0xDD86

struct tunhdr_t {
    uint16_t flags;
    uint16_t proto;
} __attribute__ ((__packed__));

struct ipv4hdr_t {
    uint8_t version: 4;
    uint8_t ihl: 4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t iden;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t proto;
    uint16_t chksum;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__ ((__packed__));

#endif

