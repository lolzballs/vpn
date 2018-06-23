#include "packethdr.h"
#include "tun.h"
#include "util.h"
#include "trie.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

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

struct conn_t {
    uint32_t id;
    bool connected;
    char allowed_ips[64]; // in cidr format
    struct sockaddr udp_addr;
    time_t last_packet;
};

struct server_conn_t {
    uint32_t id;
    struct sockaddr udp_addr;
};

struct clients_t {
    struct conn_t *clients;
    ssize_t len;

    struct node_t *trie;
};

struct vpn_t {
    struct config_t config;
    struct tun_t *tun;
    uv_udp_t *handle;
    union {
        struct clients_t clients; // is_server == true
        struct server_conn_t server; // is_server == false
    };
};

void clients_init(struct clients_t *clients) {
    // TODO: use configuration file, entire function is not production ready!
    clients->clients = malloc(sizeof(struct conn_t) * 1);
    clients->len = 1;
    clients->trie = trie_new();
    
    strcpy(clients->clients[0].allowed_ips, "10.69.0.2/32");

    struct sockaddr_in vpn_addr;
    uint32_t mask;
    cidr_parse(clients->clients[0].allowed_ips, (struct sockaddr*) &vpn_addr, &mask);
    clients->clients[0].id = 0;
    clients->clients[0].connected = false;
    trie_map(clients->trie, vpn_addr.sin_addr.s_addr, mask, &clients->clients[0]);
}

void clients_connect(struct clients_t *clients, uint32_t id, const struct sockaddr *sa) {
    // TODO: Make this safe and replace incrementing ids
    struct conn_t *client = &clients->clients[id];
    client->connected = true;
    memcpy(&client->udp_addr, sa, sizeof(struct sockaddr));

    if (sa->sa_family != AF_INET) {
        // struct sockaddr_in *sa4 = (struct sockaddr_in*) sa;
        fprintf(stderr, "ipv6 is currently unsupported\n");
    }
}

void clients_disconnect(struct clients_t *clients, uint32_t id) {
    // TODO: same as in clients_connect
    struct conn_t *client = &clients->clients[id];
    client->connected = false;

    printf("client %d disconnect\n", id);
}

void timeout_check(uv_timer_t *handle) {
    printf("timeout check\n");
    struct vpn_t *vpn = handle->data;
    struct clients_t *clients = &vpn->clients;
    time_t now = time(NULL);

    for (ssize_t i = 0; i < clients->len; i++) {
        if (now - clients->clients[i].last_packet >= vpn->config.timeout &&
                clients->clients[i].connected) {
            clients_disconnect(clients, i);
        }
    }
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

    free(handle->data);
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
    
    // TODO: Change format when crypto implemented
    if ((res = tun_read(vpn->tun, data + sizeof(uint32_t), 2048 - sizeof(uint32_t))) == -1) {
        perror("error reading");
        return;
    }
    buf.base = (char*) data;
    buf.len = res;

    if (vpn->config.mode == CONFIG_MODE_SERVER) {
        struct clients_t *clients = &vpn->clients;

        struct tunhdr_t *tunhdr = (struct tunhdr_t*) data;
        if (tunhdr->proto != ETH_P_IPv4)
            return;

        struct ipv4hdr_t *hdr = (struct ipv4hdr_t*) (data + sizeof(struct tunhdr_t));
        struct conn_t *client = trie_find(clients->trie, hdr->daddr, 32);
        if (client != NULL) {
            send_req = malloc(sizeof(uv_udp_send_t));
            send_req->data = data;

            *data = client->id;

            if (uv_udp_send(send_req, vpn->handle, &buf, 1, &client->udp_addr, socket_sent_server) < 0) {
                fprintf(stderr, "error sending to client, %s\n", uv_strerror(res));
            }
        }
    } else {
        send_req = malloc(sizeof(uv_udp_send_t));
        send_req->data = data;

        *data = vpn->server.id;

        if (uv_udp_send(send_req, vpn->handle, &buf, 1, &vpn->server.udp_addr, socket_sent_client) < 0) {
            fprintf(stderr, "error sending to server, %s\n", uv_strerror(res));
        }
    }
}

void read_socket(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    if (nread < 0) {
        fprintf(stderr, "error reading udp socket: %s\n", uv_strerror(nread));
        uv_close((uv_handle_t*) handle, NULL);
        goto ret;
    }

    // Nothing to read
    if (nread == 0 && addr == NULL) {
        goto ret;
    }

    // TODO: Change this when crypto implemented
    if (nread < 4) {
        goto ret;
    }
    uint32_t id = ntohl(*(uint32_t*) buf->base);
    char *payload = buf->base + sizeof(uint32_t);

    // TODO: Verify that tun is writable
    struct vpn_t *vpn = handle->data;
    tun_write(vpn->tun, (uint8_t*) payload, nread);

    if (vpn->config.mode == CONFIG_MODE_SERVER) {
        if (id >= vpn->clients.len)
            goto ret;

        if (!vpn->clients.clients[id].connected) {
            char buf[256];
            clients_connect(&vpn->clients, id, addr);
            uv_ip4_name((struct sockaddr_in*) addr, buf, 256); 
            printf("new client (%d): %s\n", id, buf);
        }
        vpn->clients.clients[id].last_packet = time(NULL);
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

int start_server(struct config_t config) {
    int res;
    uv_loop_t *loop = uv_default_loop();
    uv_udp_t udp_sock;
    uv_timer_t timeout;
    struct tun_t tun;
    struct vpn_t vpn = {
        .tun = &tun,
        .handle = &udp_sock,
        .config = config
    };
    clients_init(&vpn.clients);

    if (create_tun(loop, &vpn, &tun, config.ip) == -1) {
        return -1;
    }

    if ((res = uv_udp_init(loop, &udp_sock)) < 0) {
        fprintf(stderr, "failed to init udp socket: %s\n", uv_strerror(res));
        return res;
    }
    if ((res = uv_udp_bind(&udp_sock, (struct sockaddr*) &config.addr, 0)) < 0) {
        fprintf(stderr, "failed to bind udp socket: %s\n", uv_strerror(res));
        return res;
    }
    udp_sock.data = &vpn;

    uv_udp_recv_start(&udp_sock, alloc_buf, read_socket);

    if ((res = uv_timer_init(loop, &timeout)) < 0) {
        fprintf(stderr, "failed to init timeout timer: %s\n", uv_strerror(res));
        return res;
    }
    timeout.data = &vpn;

    uv_timer_start(&timeout, timeout_check, 0, config.timeout * 1000);

    return uv_run(loop, UV_RUN_DEFAULT);
}

int start_client(struct config_t config) {
    int res;
    uv_loop_t *loop = uv_default_loop();
    uv_udp_t udp_sock;
    struct tun_t tun;
    struct vpn_t vpn = {
        .tun = &tun,
        .handle = &udp_sock,
        .server = {
            .id = 0,
            .udp_addr = *((struct sockaddr*) &config.addr)
        },
        .config = config
    };

    if (create_tun(loop, &vpn, &tun, config.ip) == -1) {
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
    int res, ipind;
    char ch;
    struct sockaddr_in server_addr;
    struct config_t config = {
        .mode = CONFIG_MODE_UNSET,
        .timeout = 10,
    };

    // TODO: evaluate safety of this
    while ((ch = getopt_long(argc, argv, "cst:", arguments, NULL)) != -1) {
        switch (ch) {
            case 's':
                config.mode = CONFIG_MODE_SERVER;
                break;
            case 'c':
                config.mode = CONFIG_MODE_CLIENT;
                break;
            case 't':
                config.timeout = strtol(optarg, NULL, 10);
                if (errno != 0) {
                    goto usage;
                }
                break;
            default:
                goto usage;
        }
    }

    ipind = optind;

    if (config.mode == CONFIG_MODE_UNSET)
        goto usage;

    if (argc != ipind + 3)
        goto usage;

    if ((res = uv_ip4_addr(argv[ipind + 1], atoi(argv[ipind + 2]), &server_addr)) < 0) {
        fprintf(stderr, "specified ip invalid: %s\n", uv_strerror(res));
        goto usage;
    }

    config.addr = server_addr;
    strncpy(config.ip, argv[ipind], 64);

    if (config.mode == CONFIG_MODE_SERVER) {
        return start_server(config);
    } else if (config.mode == CONFIG_MODE_CLIENT) {
        return start_client(config);
    }

    return 0;
    
usage:
    printf("usage: %s [-t timeout] {-s|-c} <tun_ip> <ip> <port>\n", argv[0]);
    return -1;
}

