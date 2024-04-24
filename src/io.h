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

typedef void (*op_callback_t)(void *ctx, struct op *op, struct io_uring_cqe *cqe);

struct op
{
    op_callback_t callback;
};

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
int io_tick(struct io *io, void *ctx);
void io_send(struct io *io, int fd, void *buf, unsigned int buf_len, struct op *op, op_callback_t callback);
void io_sendto(struct io *io, int fd, void *buf, unsigned int buf_len, struct sockaddr *addr, unsigned int addr_len, struct op *op, op_callback_t callback);
void io_recv(struct io *io, int fd, void *buf, unsigned long len, struct op *op, op_callback_t callback);
void io_recvmsg_multi_bg(struct io *io, int fd, int bgid, struct msghdr *msg, struct op *op, op_callback_t callback);
void io_connect(struct io *io, int fd, struct sockaddr *addr, unsigned int addr_len, struct op *op, op_callback_t callback);

#endif // IO_H
