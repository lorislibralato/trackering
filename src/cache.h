#ifndef CACHE_H
#define CACHE_H

#include "memory.h"

struct cache
{
    struct page *next;
    unsigned int count;
    unsigned short obj_size;
};

void cache_init(struct cache *cache, unsigned int size);

struct page *mm_pool_cache_get_slab(struct mm_pool *mm, struct cache *cache);

void *mm_pool_cache_get(struct mm_pool *mm, struct cache *cache);

void mm_pool_cache_put(struct mm_pool *mm, struct cache *cache, void *obj);

#endif // CACHE_H