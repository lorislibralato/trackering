#ifndef BTREE_H
#define BTREE_H

#include "packet.h"
#include "memory.h"
#include "cache.h"
#include "list.h"
#include "../c-rbtree/src/c-rbtree.h"

#define EXPIRE_TIME (15 * 60 * 1000)

struct tree;
struct torrent_node;
struct peer_node;

struct tree
{
    struct mm_pool *mm;
    struct cache peer_node_cache;
    struct cache torrent_node_cache;
    CRBTree rb;
    struct list_head lru;
    unsigned int count;
};

struct torrent_node
{
    CRBNode node;
    struct list_head torrent_lru;
    struct list_head peer_lru;
    CRBTree peer_rb;
    infohash_t infohash;
    unsigned short peers;
    unsigned short seeders;
};

struct peer_node
{
    CRBNode node;
    struct list_head lru;
    struct peer peer;
    unsigned int ms;
    unsigned char seeder;
};

void tree_init(struct tree *tree, struct mm_pool *mm);
struct torrent_node *torrent_lookup(struct tree *tree, infohash_t *infohash);
struct torrent_node *torrent_lookup_or_insert(struct tree *tree, infohash_t *infohash);
// remove the last accessed peer if it has not been refreshed in the last n minutes
void torrent_gc(struct tree *tree, struct torrent_node *torrent);
// add a peer if it doesn't exist else update its timestamp
void peer_refresh(struct tree *tree, struct torrent_node *torrent, struct peer peer, unsigned int ms);

#endif