#include "tun.h"

#include <linux/if.h>
#include <linux/if_tun.h>

int tun_init(uv_loop_t *loop, struct tun_t *tun) {
    int res;

    memset(&tun->ifr, 0, sizeof(struct ifreq));
    if ((tun->fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("error opening /dev/net/tun");
        return -1;
    }

    tun->ifr.ifr_flags = IFF_TUN;
    if ((res = ioctl(tun->fd, TUNSETIFF, (void*) &tun->ifr)) < 0) {
        perror("error creating tuntap device (TUNSETIFF)");
        return -1;
    }

    if ((res = uv_poll_init(loop, &tun->uv_handle, tun->fd)) < 0) {
        fprintf(stderr, "uv_poll_init (tun): %s\n", uv_strerror(res));
        return -1;
    }

    return 0;
}

int tun_close(struct tun_t *tun) {
    return close(tun->fd);
}

int tun_write(struct tun_t *tun, const uint8_t *buf, size_t len) {
    return write(tun->fd, buf, len);
}

int tun_read(struct tun_t *tun, uint8_t *buf, size_t len) {
    return read(tun->fd, buf, len);
}

uv_poll_t* tun_handle(struct tun_t *tun) {
    return &tun->uv_handle;
}

