#ifndef trie_h
#define trie_h

#include <stdint.h>
#include <stdlib.h>

struct conn_t;

struct node_t {
    struct conn_t *client;
    size_t sublen;
    struct node_t *ch[2];
};

static struct node_t *create_node();
static int map_client(struct node_t **n, uint32_t address, uint32_t mask, struct conn_t *client, size_t depth);
static struct conn_t *find_client(struct node_t *n, uint32_t address, uint32_t mask, size_t depth);
static int unmap_client(struct node_t **n, uint32_t address, uint32_t mask, size_t depth);

struct node_t *trie_new();
int trie_map(struct node_t *t, uint32_t address, uint32_t mask, struct conn_t *client);
struct client_t *trie_find(struct node_t *t, uint32_t address, uint32_t mask);
int trie_free(struct node_t *t);

#endif

