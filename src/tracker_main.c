#include <liburing.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include "list.h"
#include "memory.h"
#include "cache.h"
#include "tracker.h"
#include "utils.h"

static struct tracker trk;

/*
    int a = 1321;

    int *p1 = &a;
    int *p2 = (int *)(((unsigned long)&a) & ((1UL << 48) - 1) | (1UL << 62));

    int p2_val = *(int *)((unsigned long)p2 & ((~0UL) >> 16));
    printf("%x %x\n", p1, p2);
    printf("%d %d\n", *p1, p2_val);
    assert(*p1 == p2_val);
*/

int main()
{
    int ret;

    unsigned long cpus = 2;
    unsigned long ram = 516 * (1L << 20);
    unsigned int pages = (unsigned int)(ram >> PAGE_SIZE_BITS);
    unsigned int pages_per_core = pages / cpus;

    printf("ram: %lu pages: %d\n", ram, pages);

    void *buf;
    ret = set_locked_limit(RLIM_INFINITY);
    assert(!ret);

    ret = alloc_locked_pages(pages, &buf);
    if (ret)
    {
        printf("error allocating the memory -> ret: %d, errno: %d - %s\n", ret, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    tracker_init(&trk, buf + pages_per_core * PAGE_SIZE * 0, pages_per_core);
    tracker_tick(&trk);

    return ret;
}
