#ifndef MEMORY_H
#define MEMORY_H

#define PAGE_SIZE_BITS (12)
#define PAGE_SIZE (1 << PAGE_SIZE_BITS)
#define PAGE_MASK ~(PAGE_SIZE - 1)

struct page
{
    struct page *next;
    union
    {
        // slab
        struct
        {
            unsigned int free_count;
            unsigned short next_obj;
        };
    };
};

struct mm_pool
{
    struct page *descs;
    void *page_start;

    struct page *next;
    unsigned int free_count;
    unsigned int pages;
};

int set_locked_limit(unsigned long max);
int alloc_locked_pages(unsigned int pages, void **buf);
void mm_pool_init(struct mm_pool *mm, void *addr, unsigned int pages);
void *mm_pool_page_to_addr(struct mm_pool *mm, struct page *page);
struct page *mm_pool_addr_to_page(struct mm_pool *mm, void *addr);
// return linked list of "count" pages
struct page *mm_pool_get_pages(struct mm_pool *mm, unsigned int count);
struct page *mm_pool_get_page(struct mm_pool *mm);

#endif // MEMORY_H