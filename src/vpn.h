#include "config.h"

#include <uv.h>
#include <stdbool.h>
#include <stdint.h>

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

void clients_init(struct clients_t *clients);
void clients_connect(struct clients_t *clients, uint32_t id, const struct sockaddr *sa);
void clients_disconnect(struct clients_t *clients, uint32_t id);

static void alloc_buf(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void timeout_check(uv_timer_t *handle);
static void socket_sent(uv_udp_send_t *handle, int status);
static void read_tunnel(uv_poll_t *handle, int status, int events);
static void read_socket(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);

static int create_tun(uv_loop_t *loop, struct vpn_t *vpn, struct tun_t *tun, char *ip);
static int start_server(struct config_t config);
static int start_client(struct config_t config);
int vpn_start(struct config_t config);

