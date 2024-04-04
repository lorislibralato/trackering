#if !defined(TRACKER_H)
#define TRACKER_H

#include "memory.h"
#include "cache.h"
#include "io.h"
#include "liburing.h"

#define RECV_BG_BUF_LEN (2048)
#define RECV_BG_ENTRIES (1 << 15)

enum bufgroup_ids
{
    BG_ID_RECV = 0,
    BG_ID_COUNT
};

#define MAX_RESPONSE_ORDER (11)
#define MIN_RESPONSE_ORDER (6)
#define SEND_CACHE_COUNT (MAX_RESPONSE_ORDER - MIN_RESPONSE_ORDER + 1)

struct buf_group
{
    struct io_uring_buf_ring *desc;
    void *buf_start;
};

struct tracker
{
    struct io io;

    struct mm_pool mm;

    struct cache send_caches[SEND_CACHE_COUNT + 1];

    struct op_recv op_recv;
    struct buf_group group[BG_ID_COUNT];
    int fd;
};

void tracker_init(struct tracker *trk, void *buf, unsigned int pages);
int tracker_tick(struct tracker *trk);
void start_receiving(struct tracker *trk);
void received_msg(struct tracker *trk, struct op *_op, struct io_uring_cqe *cqe);

#endif // TRACKER_H
