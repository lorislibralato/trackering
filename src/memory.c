#include <unistd.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "memory.h"
#include "cache.h"

int set_locked_limit(unsigned long max)
{
    struct rlimit limit;
    limit.rlim_cur = max;
    limit.rlim_max = max;
    return setrlimit(RLIMIT_MEMLOCK, &limit);
}

int alloc_locked_pages(unsigned int pages, void **buf)
{
    int ret;
    
    void *ptr_ret;
    ptr_ret = mmap(NULL, pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_LOCKED, -1, 0);
    if (ptr_ret == MAP_FAILED)
        return -1;

    ret = mlock(ptr_ret, pages * PAGE_SIZE);
    if (ret)
    {
        munmap(ptr_ret, pages * PAGE_SIZE);
        return ret;
    }

    *buf = ptr_ret;
    return ret;
}

void mm_pool_init(struct mm_pool *mm, void *addr, unsigned int pages)
{
    unsigned long header_size = (sizeof(struct page) * pages + (PAGE_SIZE - 1)) & PAGE_MASK;
    mm->descs = addr;
    mm->pages = pages;
    mm->page_start = addr + header_size;
    mm->free_count = pages;
    mm->next = addr;

    for (unsigned int i = 0; i < pages - 1; i++)
        mm->descs[i].next = &mm->descs[i + 1];

    mm->descs[pages - 1].next = NULL;
}

void *mm_pool_page_to_addr(struct mm_pool *mm, struct page *page)
{
    unsigned int i = (page - mm->descs) / sizeof(struct page);
    return mm->page_start + i * PAGE_SIZE;
}

struct page *mm_pool_addr_to_page(struct mm_pool *mm, void *addr)
{
    unsigned int i = (addr - mm->page_start) / PAGE_SIZE;
    return &mm->descs[i];
}

// return linked list of "count" pages
struct page *mm_pool_get_pages(struct mm_pool *mm, unsigned int count)
{
    if (mm->free_count < count)
        return NULL;

    struct page *start = mm->next;
    struct page **tmp = tmp = &start;

    for (unsigned int i = 0; i < count; ++i, tmp = &((*tmp)->next))
        ;

    mm->next = *tmp;
    *tmp = NULL;
    mm->free_count -= count;

    return start;
}

struct page *mm_pool_get_page(struct mm_pool *mm)
{
    if (!mm->free_count)
        return NULL;

    struct page *page = mm->next;

    // free_count > 0 => page->next != NULL
    mm->next = page->next;
    mm->free_count--;
    page->next = NULL;

    return page;
}