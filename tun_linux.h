#ifndef tun_linux_h
#define tun_linux_h

#include <uv.h>

#include <unistd.h>

#include <linux/if.h>
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
    uv_poll_t uv_handle;
    struct ifreq ifr;
    int fd;
};

#endif
