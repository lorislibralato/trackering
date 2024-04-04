#if !defined(IO_H)
#define IO_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include "liburing.h"

struct tracker;
struct op;

struct io
{
    struct io_uring_params params;
    struct io_uring ring;
};

typedef void (*op_callback_t)(struct tracker *trk, struct op *_op, struct io_uring_cqe *cqe);

struct op
{
    op_callback_t callback;
};

struct op_recv
{
    struct op op;
    struct msghdr msg;
};

struct op_send
{
    struct op op;
    union
    {
        struct sockaddr addr;
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
    };
    char cache_index;
    char buf[];
};
static_assert(sizeof(struct op_send) == 8 + 28 + 4, "");

struct io_options
{
    unsigned int entries;
    unsigned int cqe_order;
    unsigned int sq_idle;
    int sq_cpu;
    int attach_wq;
    char use_direct_files;
};

int io_init(struct io *io, struct io_options *options);
int io_tick(struct io *io, struct tracker *trk);
void io_recvmsg_multi(struct io *io, int fd, int bgid, struct msghdr *msg, struct op *op, op_callback_t callback);
void io_sendmsg(struct io *io, int fd, void *buf, unsigned int buf_len, struct sockaddr *addr, unsigned int addr_len, struct op *op, op_callback_t callback);

#endif // IO_H
