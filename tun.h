#ifdef __linux__
#include "tun_linux.h"
#endif

#include <uv.h>

#include <string.h>
#include <stdlib.h>

struct tun_t;

int tun_init(uv_loop_t *loop, struct tun_t *tun);
int tun_close(struct tun_t *tun);
int tun_up(struct tun_t *tun);
int tun_addr(struct tun_t *tun, char *ip);
int tun_write(struct tun_t *tun, const uint8_t *buf, size_t len);
int tun_read(struct tun_t *tun, uint8_t *buf, size_t len);
uv_poll_t* tun_handle(struct tun_t *tun);

