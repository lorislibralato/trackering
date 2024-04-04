#include <unistd.h>
#include "cache.h"

void cache_init(struct cache *cache, unsigned int size)
{
    cache->obj_size = size;
    cache->next = NULL;
    cache->count = 0;
}

struct page *mm_pool_cache_get_slab(struct mm_pool *mm, struct cache *cache)
{
    struct page *page = mm_pool_get_page(mm);
    if (!page)
        return NULL;

    void *page_data = mm_pool_page_to_addr(mm, page);
    page->free_count = PAGE_SIZE / cache->obj_size;
    page->next_obj = 0;

    for (unsigned int i = 0; i < page->free_count; i++)
        *(unsigned short *)(page_data + i * cache->obj_size) = i + 1;

    return page;
}

void *mm_pool_cache_get(struct mm_pool *mm, struct cache *cache)
{
    struct page *page = cache->next;
    if (!page)
    {
        page = mm_pool_cache_get_slab(mm, cache);
        // TODO: handle page reclaim
        if (!page)
            return NULL;

        page->next = cache->next;
        cache->next = page;
    }
    else if (page->free_count == 1)
    {
        cache->next = page->next;
        // UB in page->next
        cache->count--;
    }

    void *page_data = mm_pool_page_to_addr(mm, page);
    void *obj = page_data + page->next_obj * cache->obj_size;

    page->free_count--;

    return obj;
}

void mm_pool_cache_put(struct mm_pool *mm, struct cache *cache, void *obj)
{
    struct page *page = (struct page *)((unsigned long)obj & PAGE_MASK);
    void *page_data = mm_pool_page_to_addr(mm, page);

    unsigned short i = (unsigned long)(obj - page_data) / cache->obj_size;

    *(unsigned short *)(obj) = page->next_obj;
    page->next_obj = i;

    page->free_count++;

    if (page->free_count == 1)
    {
        // TODO: put at the end of cache slab list
        page->next = cache->next;
        cache->next = page;
    }
}