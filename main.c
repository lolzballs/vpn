#include "tun.h"
#include "util.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct sent_data_t {
    uv_buf_t *data;
    size_t idx;
    size_t clients;
};

struct conn_t {
    struct sockaddr sa;
};

struct clients_t {
    struct conn_t **clients;
    ssize_t len;
};

struct vpn_t {
    bool is_server;
    struct tun_t *tun;
    uv_udp_t *handle;
    union {
        struct clients_t clients; // is_server == true
        struct conn_t server; // is_server == false
    };
};

ssize_t clients_add(struct clients_t *clients) {
    struct conn_t *new = malloc(sizeof(struct conn_t));

    clients->clients[clients->len] = new;
    return clients->len++;
}

void alloc_buf(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(2048);
    buf->len = 2048;
}

void socket_sent_server(uv_udp_send_t *handle, int status) {
    if (status < 0) {
        fprintf(stderr, "error sending on udp socket: %s\n", uv_strerror(status));
        return;
    }

    shared_ptr_release(handle->data);
    free(handle);
}

void socket_sent_client(uv_udp_send_t *handle, int status) {
    if (status < 0) {
        fprintf(stderr, "error sending on udp socket: %s\n", uv_strerror(status));
        return;
    }

    free(handle->data);
    free(handle);
}

void read_tunnel(uv_poll_t *handle, int status, int events) {
    int res;
    uv_buf_t buf;
    uv_udp_send_t *send_req;
    struct vpn_t *vpn = handle->data;
    uint8_t *data = malloc(2048);

    if (status < 0) {
        fprintf(stderr, "tunnel error: %s\n", uv_strerror(status));
        return;
    }

    if (events != UV_READABLE) {
        return;
    }

    if ((res = tun_read(vpn->tun, data, 2048)) == -1) {
        perror("error reading");
        return;
    }
    buf.base = (char*) data;
    buf.len = res;

    if (vpn->is_server) {
        struct clients_t clients = vpn->clients;
        struct shared_ptr_t *data_ptr = shared_ptr_wrap(data, clients.len);
        for (ssize_t i = 0; i < clients.len; i++) {
            send_req = malloc(sizeof(uv_udp_send_t));
            send_req->data = data_ptr;

            struct conn_t *client = clients.clients[i];
            if (uv_udp_send(send_req, vpn->handle, &buf, 1, &client->sa, socket_sent_server) < 0) {
                fprintf(stderr, "error sending to client %ld, %s\n", i, uv_strerror(res));
            }
        }
    } else {
        struct conn_t *server = &vpn->server;
        send_req = malloc(sizeof(uv_udp_send_t));
        send_req->data = data;
        if (uv_udp_send(send_req, vpn->handle, &buf, 1, &server->sa, socket_sent_client) < 0) {
            fprintf(stderr, "error sending to server, %s\n", uv_strerror(res));
        }
    }
}

void read_socket(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    if (nread < 0) {
        fprintf(stderr, "error reading udp socket: %s\n", uv_strerror(nread));
        uv_close((uv_handle_t*) handle, NULL);
        free(buf->base);
        return;
    }

    // Nothing to read
    if (nread == 0 && addr == NULL) {
        goto ret;
    }

    // TODO: Verify that tun is writable
    struct vpn_t *vpn = handle->data;
    tun_write(vpn->tun, (uint8_t*) buf->base, nread);

    if (vpn->is_server) {
        struct clients_t *clients = &vpn->clients;
        for (ssize_t i = 0; i < clients->len; i++) {
            struct conn_t *client = clients->clients[i];
            if (sockaddr_cmp(&client->sa, addr)) {
                goto ret;
            }
        }

        char buf[256];
        ssize_t idx = clients_add(clients);
        memcpy(&clients->clients[idx]->sa, addr, sizeof(struct sockaddr));
        uv_ip4_name((struct sockaddr_in*) addr, buf, 256); 
        printf("new client: %s\n", buf);
        printf("total clients: %ld\n", clients->len);
    }

ret:
    free(buf->base);
}

int create_tun(uv_loop_t *loop, struct vpn_t *vpn, struct tun_t *tun, char *ip) {
    if (tun_init(loop, tun) == -1) {
        return -1;
    }
    if (tun_up(tun) == -1) {
        return -1;
    }
    if (tun_addr(tun, ip) == -1) {
        return -1;
    }
    tun->uv_handle.data = vpn;

    uv_poll_start(&tun->uv_handle, UV_READABLE, read_tunnel);

    return 0;
}

int start_server(struct sockaddr_in server_addr, char *ip) {
    int res;
    uv_loop_t *loop = uv_default_loop();
    uv_udp_t udp_sock;
    struct tun_t tun;
    struct conn_t *clients[256];
    struct vpn_t vpn = {
        .is_server = true,
        .tun = &tun,
        .handle = &udp_sock,
        .clients = {
            .clients = clients,
            .len = 0,
        },
    };

    if (create_tun(loop, &vpn, &tun, ip) == -1) {
        return -1;
    }

    if ((res = uv_udp_init(loop, &udp_sock)) < 0) {
        fprintf(stderr, "failed to init udp socket: %s\n", uv_strerror(res));
        return res;
    }
    if ((res = uv_udp_bind(&udp_sock, (struct sockaddr*) &server_addr, 0)) < 0) {
        fprintf(stderr, "failed to bind udp socket: %s\n", uv_strerror(res));
        return res;
    }
    udp_sock.data = &vpn;

    uv_udp_recv_start(&udp_sock, alloc_buf, read_socket);

    return uv_run(loop, UV_RUN_DEFAULT);
}

int start_client(struct sockaddr_in server_addr, char *ip) {
    int res;
    uv_loop_t *loop = uv_default_loop();
    uv_udp_t udp_sock;
    struct tun_t tun;
    struct vpn_t vpn = {
        .is_server = false,
        .tun = &tun,
        .handle = &udp_sock,
        .server = {
            .sa = *((struct sockaddr*) &server_addr),
        }
    };

    if (create_tun(loop, &vpn, &tun, ip) == -1) {
        return -1;
    }

    if ((res = uv_udp_init(loop, &udp_sock)) < 0) {
        fprintf(stderr, "failed to init udp socket: %s\n", uv_strerror(res));
        return res;
    }
    udp_sock.data = &vpn;

    uv_udp_recv_start(&udp_sock, alloc_buf, read_socket);

    return uv_run(loop, UV_RUN_DEFAULT);
}

int main(int argc, char *argv[]) {
    int res;
    struct sockaddr_in server_addr;

    if (argc != 5) {
usage:
        printf("usage: %s {-s|-c} <tun_ip> <ip> <port>\n", argv[0]);
        return -1;
    }

    if ((res = uv_ip4_addr(argv[3], atoi(argv[4]), &server_addr)) < 0) {
        fprintf(stderr, "specified ip invalid: %s\n", uv_strerror(res));
        return -1;
    }

    if (!strncmp(argv[1], "-s", 2)) {
        return start_server(server_addr, argv[2]);
    } else if (!strncmp(argv[1], "-c", 2)) {
        return start_client(server_addr, argv[2]);
    } else {
        goto usage;
    }

    return 0;
}
