#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#define SAFE_SYSTEM(cmd, ...) ({ \
        int pid, result = -1; \
        if ((pid = fork()) == 0) { \
            result = execl(cmd, cmd, __VA_ARGS__, NULL); \
            if (result == -1) { \
                perror("execl failed"); \
            } \
            exit(1); \
        } else { \
            waitpid(pid, &result, 0); \
        } \
        result; \
    })

struct tun_t {
    struct ifreq ifr;
    int fd;
};

int tun_init(struct tun_t *tun) {
    int res;

    memset(&tun->ifr, 0, sizeof(struct ifreq));
    if ((tun->fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("error opening /dev/net/tun");
        return -1;
    }

    tun->ifr.ifr_flags = IFF_TUN;
    if ((res = ioctl(tun->fd, TUNSETIFF, (void*) &tun->ifr)) < 0) {
        close(tun->fd);
        perror("error creating tuntap device (TUNSETIFF)");
        return -1;
    }

    return 0;
}

int tun_close(struct tun_t *tun) {
    return close(tun->fd);
}

int tun_up(struct tun_t *tun) {
    int res = SAFE_SYSTEM("/bin/ip", "link", "set", tun->ifr.ifr_name, "up");
    if (res != 0) {
        fputs("error occured putting tun up\n", stderr);
        return -1;
    }
    return 0;
}

int tun_addr(struct tun_t *tun, char *ip) {
    int res = SAFE_SYSTEM("/bin/ip", "addr", "add", ip, "dev", tun->ifr.ifr_name);
    if (res != 0) {
        fputs("error occured assigning ip address to tun\n", stderr);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int sockfd, res;
    struct tun_t tun;
    struct sockaddr_in addr;
    struct sockaddr sa;
    socklen_t sa_len = sizeof(sa);
    struct pollfd fds[2];

    if (argc != 5) {
usage:
        printf("usage: %s {-s|-c} <tun_ip> <ip> <port>\n", argv[0]);
        return -1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("error creating udp socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[4]));
    if ((res = inet_pton(AF_INET, argv[3], &addr.sin_addr)) == -1) {
        perror("error with inet_pton");
        return -1;
    }

    if (!strncmp(argv[1], "-s", 2)) {
        if ((res = bind(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1) {
            perror("error binding to port");
            return -1;
        }

        if ((res = recvfrom(sockfd, NULL, 0, MSG_PEEK, &sa, &sa_len)) == -1) {
            perror("error getting client ip");
            return -1;
        }

        char buf[32];
        if (sa.sa_family == AF_INET) {
            struct sockaddr_in *sa4 = (struct sockaddr_in*) &sa;
            if (inet_ntop(AF_INET, &sa4->sin_addr, buf, 32) == NULL) {
                perror("inet_ntop");
                return -1;
            }
            printf("connected to %s:%d\n", buf, ntohs(sa4->sin_port));
        } else {
            fputs("wrong sa_family\n", stderr);
            return -1;
        }
    } else if (!strncmp(argv[1], "-c", 2)) {
        sa = *((struct sockaddr*) &addr);
        sa_len = sizeof(struct sockaddr);
    } else {
        goto usage;
    }

    if ((res = tun_init(&tun)) == -1) {
        return -1;
    }
    if ((res = tun_up(&tun)) == -1) {
        return -1;
    }
    if ((res = tun_addr(&tun, argv[2])) == -1) {
        return -1;
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    fds[0].fd = tun.fd;
    fds[0].events = POLLOUT | POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLOUT | POLLIN;

    while (1) {
        if ((res = poll(fds, 2, 5000)) == -1) {
            perror("poll");
        }

        if (fds[0].revents & POLLIN && fds[1].revents & POLLOUT) {
            // TUN -> SOCK
            uint8_t buf[2048];
            res = read(tun.fd, buf, 2048);
            if (res < 0) {
                perror("error reading");
                return -1;
            }

            if ((res = sendto(sockfd, buf, res, 0, (struct sockaddr*) &sa, sa_len)) == -1) {
                perror("error sending");
            }
        }

        if (fds[1].revents & POLLIN && fds[0].revents & POLLOUT) {
            // SOCK -> TUN
            uint8_t buf[2048];
            res = read(sockfd, buf, 2048);
            write(tun.fd, buf, res);
        }
    }
    
    return 0;
}
