#include <stdlib.h>
#include "tree.h"

void tree_init(struct tree *tree, struct mm_pool *mm)
{
    tree->mm = mm;
    cache_init(&tree->torrent_node_cache, sizeof(struct torrent_node));
    cache_init(&tree->peer_node_cache, sizeof(struct peer_node));
    INIT_LIST_HEAD(&tree->lru);
    c_rbtree_init(&tree->rb);
    tree->count = 0;
}

struct torrent_node *torrent_lookup(struct tree *tree, infohash_t *infohash)
{
    struct torrent_node *i;

    i = c_rbnode_entry(tree->rb.root, struct torrent_node, node);
    while (i)
    {
        if (i->infohash < *infohash)
            i = i->node.left;
        else if (i->infohash > *infohash)
            i = i->node.right;
        else
            return i;
    }

    return NULL;
}

struct torrent_node *torrent_lookup_or_insert(struct tree *tree, infohash_t *infohash)
{
    struct torrent_node **i;
    struct torrent_node *p;
    struct torrent_node *new;

    // TODO: fix entry
    i = &tree->rb.root;
    p = NULL;
    while (*i)
    {
        p = *i;
        if ((*i)->infohash < *infohash)
            i = &(*i)->node.left;
        else if ((*i)->infohash > *infohash)
            i = &(*i)->node.right;
        else
            break;
    }

    if ((*i)->infohash == *infohash)
        return *i;

    new = mm_pool_cache_get(&tree->mm, &tree->torrent_node_cache);
    assert(new);

    c_rbtree_add(&tree->rb, p, i, new);
    return new;
}

void torrent_gc(struct tree *tree, struct torrent_node *torrent)
{
    // TODO: read current timestamp
    unsigned int now;

    unsigned short deleted = 0;
    unsigned short seeders = 0;

    struct peer_node *peer;
    list_for_each_entry(peer, &torrent->peer_lru, lru)
    {
        if (peer->ms + EXPIRE_TIME < now)
            break;

        deleted += 1;
        seeders += peer->seeder;
        list_del(&peer->lru);
        // TODO: remove from rbtree
        c_rbnode_unlink_stale(&peer->node);
        mm_pool_cache_put(&tree->mm, &tree->peer_node_cache, peer);
    }

    torrent->seeders -= seeders;
    torrent->peers -= deleted;
}

void peer_refresh(struct tree *tree, struct torrent_node *torrent, struct peer peer, unsigned int ms)
{
    struct peer_node *found;
    // search peer

    found->ms = ms;
    // if already existed
    if (1)
        list_del(&found->lru);

    list_add(&found->lru, &torrent->peer_lru);
}