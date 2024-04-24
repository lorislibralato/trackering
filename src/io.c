#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "io.h"

int io_init(struct io *io, struct io_options *opt)
{
    int ret;

    memset(&io->params, 0, sizeof(io->params));
    io->params.sq_thread_idle = opt->sq_idle;
    io->params.cq_entries = opt->entries << opt->cqe_order;
    io->params.flags = IORING_SETUP_SQPOLL | IORING_SETUP_NO_SQARRAY | IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_CQSIZE;

    if (opt->sq_cpu != -1)
    {
        io->params.flags |= IORING_SETUP_SQ_AFF;
        io->params.sq_thread_cpu = opt->sq_cpu;
    }

    if (opt->attach_wq != -1)
    {
        io->params.wq_fd = opt->attach_wq;
        io->params.flags |= IORING_SETUP_ATTACH_WQ;
    }

    ret = io_uring_queue_init_params(opt->entries, &io->ring, &io->params);
    if (ret)
        return ret;

    if (opt->use_direct_files)
    {
        ret = io_uring_register_files_sparse(&io->ring, 16);
        return ret;
    }

    return 0;
}

int io_tick(struct io *io, void *ctx)
{
    struct __kernel_timespec ts;
    struct io_uring_cqe *cqe;
    struct op *op;
    unsigned int head;
    unsigned int count;
    int ret;

    ts.tv_nsec = 1 * 1000 * 1000;
    ts.tv_sec = 0;

    ret = io_uring_submit_and_wait_timeout(&io->ring, &cqe, 1, &ts, NULL);
    if (ret == -ETIME)
        return 0;
    if (ret < 0)
        return ret;

    count = 0;
    io_uring_for_each_cqe(&io->ring, head, cqe)
    {
        if (cqe->res < 0)
        {
            printf("cqe err: %d - %s\n", -cqe->res, strerror(-cqe->res));
        }
        else
        {
            op = (struct op *)cqe->user_data;
            op->callback(ctx, op, cqe);
        }

        count++;
    }
    io_uring_cq_advance(&io->ring, count);

    return 0;
}

struct io_uring_sqe *io_prepare_sqe(struct io *io, struct op *op, op_callback_t callback)
{
    struct io_uring_sqe *sqe;
    sqe = io_uring_get_sqe(&io->ring);
    if (!sqe)
        return NULL;

    op->callback = callback;
    io_uring_sqe_set_data(sqe, op);

    return sqe;
}

void io_recvmsg_multi_bg(struct io *io, int fd, int bgid, struct msghdr *msg, struct op *op, op_callback_t callback)
{
    struct io_uring_sqe *sqe = io_prepare_sqe(io, op, callback);
    assert(sqe);

    io_uring_prep_recvmsg_multishot(sqe, fd, msg, 0);
    sqe->flags |= IOSQE_BUFFER_SELECT;
    sqe->buf_group = bgid;
}

void io_recv(struct io* io, int fd, void* buf, unsigned long len, struct op* op, op_callback_t callback) {
    struct io_uring_sqe *sqe = io_prepare_sqe(io, op, callback);
    assert(sqe);

    io_uring_prep_recv(sqe, fd, buf, len, 0);
}

void io_sendto(struct io *io, int fd, void *buf, unsigned int buf_len, struct sockaddr *addr, unsigned int addr_len, struct op *op, op_callback_t callback)
{
    struct io_uring_sqe *sqe = io_prepare_sqe(io, op, callback);
    assert(sqe);

    io_uring_prep_sendto(sqe, fd, buf, buf_len, 0, addr, addr_len);
}

void io_connect(struct io *io, int fd, struct sockaddr *addr, unsigned int addr_len, struct op *op, op_callback_t callback) {
    struct io_uring_sqe *sqe = io_prepare_sqe(io, op, callback);
    assert(sqe);

    io_uring_prep_connect(sqe, fd, addr, addr_len);
} 

void io_send(struct io *io, int fd, void *buf, unsigned int buf_len, struct op *op, op_callback_t callback) {
    struct io_uring_sqe *sqe = io_prepare_sqe(io, op, callback);
    assert(sqe);

    io_uring_prep_send(sqe, fd, buf, buf_len, 0);
}