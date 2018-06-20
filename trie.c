#include "trie.h"

static struct node_t *create_node() {
    struct node_t *n = malloc(sizeof(struct node_t));
    n->client = NULL;
    n->ch[0] = n->ch[1] = NULL;
    n->sublen = 0;
    return n;
}

static int map_client(struct node_t **n, uint32_t address, uint32_t mask, struct client_t *client, size_t depth) {
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
        if (address & (1 << depth)) {
            res = map_client(&((*n)->ch[1]), address, mask, client, depth + 1);
        } else {
            res = map_client(&((*n)->ch[0]), address, mask, client, depth + 1);
        }

        if (res == 0)
            (*n)->sublen++;
        return res;
    }
}

static struct client_t *find_client(struct node_t *n, uint32_t address, uint32_t mask, size_t depth) {
    if (n == NULL)
        return NULL;

    if (depth == mask)
        return n->client;
    else {
        if (address & (1 << depth)) {
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
        if (address & (1 << depth)) {
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

int trie_map(struct node_t *t, uint32_t address, uint32_t mask, struct client_t *client) {
    if (mask > 32)
        return -1;

    return map_client(&t, address, mask, client, 0);
}

struct client_t *trie_find(struct node_t *t, uint32_t address, uint32_t mask) {
    if (mask > 32)
        return NULL;

    return find_client(t, address, mask, 0);
}

int trie_free(struct node_t *t) {
    // TODO: actually traverse the trie
    free(t);
    return 0;
}

