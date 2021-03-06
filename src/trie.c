#include "trie.h"

#include <assert.h>

static struct node_t *create_node() {
    struct node_t *n = malloc(sizeof(struct node_t));
    n->client = NULL;
    n->ch[0] = n->ch[1] = NULL;
    n->sublen = 0;
    return n;
}

static int map_client(struct node_t **n, uint32_t address, uint32_t mask, struct conn_t *client, size_t depth) {
    if (*n == NULL)
        *n = create_node();

    if (depth == mask) {
        if ((*n)->sublen)
            return -1;

        (*n)->client = client;
        return 0;
    } else {
        if ((*n)->client != NULL)
            return -1;

        int res = -1;
        if (address & (1 << (31 - depth))) {
            res = map_client(&((*n)->ch[1]), address, mask, client, depth + 1);
        } else {
            res = map_client(&((*n)->ch[0]), address, mask, client, depth + 1);
        }

        if (res == 0)
            (*n)->sublen++;
        return res;
    }
}

static struct conn_t *find_client(struct node_t *n, uint32_t address, uint32_t mask, size_t depth) {
    if (n == NULL)
        return NULL;

    if (depth == mask)
        return n->client;
    else {
        if (address & (1 << (31 - depth))) {
            return find_client(n->ch[1], address, mask, depth + 1);
        } else {
            return find_client(n->ch[0], address, mask, depth + 1);
        }
    }
}

static int unmap_client(struct node_t **n, uint32_t address, uint32_t mask, size_t depth) {
    if (*n == NULL)
        return -1;

    if (depth == mask) {
        if ((*n)->sublen != 1)
            return -1;

        free(*n);
        *n = NULL;
        return 0;
    } else {
        int res = -1;
        if (address & (1 << (31 - depth))) {
            res = unmap_client(&((*n)->ch[1]), address, mask, depth + 1);
        } else {
            res = unmap_client(&((*n)->ch[0]), address, mask, depth + 1);
        }
        
        if (res == 0)
            (*n)->sublen--;
        return res;
    }
}

struct node_t *trie_new() {
    return create_node();
}

int trie_map(struct node_t *t, struct vpn_addrrange_t allowed_ips, struct conn_t *client) {
    if (allowed_ips.mask > 32)
        return -1;

    // TODO: support more than ipv4
    assert(allowed_ips.addr.s4.sin_family == AF_INET);
    return map_client(&t, allowed_ips.addr.s4.sin_addr.s_addr, allowed_ips.mask, client, 0);
}

struct conn_t *trie_find(struct node_t *t, uint32_t address, uint32_t mask) {
    if (mask > 32)
        return NULL;

    return find_client(t, address, mask, 0);
}

int trie_free(struct node_t *t) {
    // TODO: actually traverse the trie
    free(t);
    return 0;
}

