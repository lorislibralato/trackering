#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <byteswap.h>
#include "tracker.h"
#include "utils.h"
#include "packet.h"

extern struct tracker trk;

void tracker_init_buf_group(struct tracker *trk, unsigned int entries, int bgid, unsigned int size)
{
    int ret;
    struct io_uring_buf_ring *br;
    br = io_uring_setup_buf_ring(&trk->io.ring, entries, bgid, 0, &ret);
    assert(!ret);

    io_uring_buf_ring_init(br);

    void *buf_start;
    unsigned int pages = ((entries * size + PAGE_SIZE - 1) & PAGE_MASK) >> PAGE_SIZE_BITS;
    ret = alloc_locked_pages(pages, &buf_start);
    assert(!ret);

    int mask = io_uring_buf_ring_mask(entries);
    for (unsigned int i = 0; i < entries; i++)
        io_uring_buf_ring_add(br, buf_start + size * i, size, i, mask, i);

    io_uring_buf_ring_advance(br, entries);

    trk->group[bgid].buf_start = buf_start;
    trk->group[bgid].desc = br;
}

void tracker_init(struct tracker *trk, void *buf, unsigned int pages)
{
    int ret;

    struct io_options opt;
    opt.entries = 1 << 10;
    opt.cqe_order = 1;
    opt.attach_wq = -1;
    opt.sq_idle = 200;
    opt.sq_cpu = 1;
    opt.use_direct_files = 0;

    ret = io_init(&trk->io, &opt);
    if (ret)
    {
        printf("error initializing the ring -> ret: %d\n", ret);
        exit(EXIT_FAILURE);
    }

    mm_pool_init(&trk->mm, buf, pages);
    cache_init(&trk->send_caches[0], sizeof(struct op_send) + sizeof(struct connect_resp));

    for (unsigned int i = 0; i < SEND_CACHE_COUNT; i++)
        cache_init(&trk->send_caches[i + 1], 1 << (MIN_RESPONSE_ORDER + i));

    tracker_init_buf_group(trk, RECV_BG_ENTRIES, BG_ID_RECV, RECV_BG_BUF_LEN);
}

void packet_sent(struct tracker *trk, struct op *op, struct io_uring_cqe *cqe)
{
    struct op_send *send = container_of(op, struct op_send, op);

    // deallocate the op (and the buffer) from the right size cache
    mm_pool_cache_put(&trk->mm, &trk->send_caches[send->cache_index], send);
}

void received_msg(struct tracker *trk, struct op *op, struct io_uring_cqe *cqe)
{
    // op is embedded inside tracker, doesn't need deallocation
    assert(&trk->op_recv.op == op);
    struct op_recv *recv;
    struct op_send *send;
    unsigned short bid;
    void *buf_addr;
    struct io_uring_recvmsg_out *hdr;
    struct sockaddr *name;
    struct header_req *req_hdr;
    unsigned int addr_size;

    recv = container_of(op, struct op_recv, op);

    // resend receive multishot if is was stopped
    if (!(cqe->flags & IORING_CQE_F_MORE))
        start_receiving(trk);

    assert(cqe->flags & IORING_CQE_F_BUFFER);
    bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
    buf_addr = trk->group[BG_ID_RECV].buf_start + bid * RECV_BG_BUF_LEN;

    hdr = io_uring_recvmsg_validate(buf_addr, cqe->res, &recv->msg);
    assert(hdr);

    name = io_uring_recvmsg_name(hdr);

    if (name->sa_family == AF_INET)
        addr_size = sizeof(struct sockaddr_in);
    else if (name->sa_family == AF_INET6)
        addr_size = sizeof(struct sockaddr_in6);
    else
    {
        printf("invalid addr family\n");
        goto cleanup;
    }

    req_hdr = io_uring_recvmsg_payload(hdr, &recv->msg);

    switch (req_hdr->action)
    {
    case Connect:
    {
        struct connect_req *req = req_hdr;
        struct connect_resp *resp;

        if (req->header.connection_id != PROTOCOL_ID)
        {
            printf("invalid connect magic const\n");
            goto cleanup;
        }

        send = mm_pool_cache_get(&trk->mm, &trk->send_caches[0]);
        assert(send);

        send->cache_index = 0;

        resp = (void *)&send->buf;
        resp->header.action = Connect;
        resp->header.transaction_id = req->header.transaction_id;
        resp->connection_id = 0; // TODO: generate this

        memcpy(&send->addr, name, addr_size);
        // printf("packet from: %s\n", inet_ntoa(((struct sockaddr_in *)name)->sin_addr));

        io_sendmsg(&trk->io, trk->fd, &send->buf, sizeof(resp), &send->addr, addr_size, &send->op, packet_sent);

        break;
    }
    case Announce:
    {
        struct announce_req *req = req_hdr;
        struct announce_resp *resp;

        switch (req->event)
        {
        default:
            break;
        }

        unsigned int found_peers = 0;

        send = mm_pool_cache_get(&trk->mm, &trk->send_caches[0]);
        assert(send);

        memcpy(&send->addr, name, addr_size);

        io_sendmsg(&trk->io, trk->fd, &send->buf, sizeof(resp), &send->addr, addr_size, &send->op, packet_sent);
        break;
    }

    case Scrape:
    {
        struct scrape_req *req = req_hdr;
        struct scrape_resp *resp;

        unsigned int payload_len = io_uring_recvmsg_payload_length(hdr, cqe->res, &recv->msg);
        unsigned int qty = (payload_len - sizeof(struct header_req)) / sizeof(req->torrents[0]);

        send = mm_pool_cache_get(&trk->mm, &trk->send_caches[0]);
        assert(send);

        io_sendmsg(&trk->io, trk->fd, &send->buf, sizeof(resp), &send->addr, addr_size, &send->op, packet_sent);
        break;
    }

    default:
        printf("invalid action\n");
        break;
    }

cleanup:
    io_uring_buf_ring_add(trk->group[BG_ID_RECV].desc, buf_addr, RECV_BG_BUF_LEN, bid, io_uring_buf_ring_mask(RECV_BG_ENTRIES), 0);
}

void start_receiving(struct tracker *trk)
{
    struct op_recv *recv = &trk->op_recv;
    memset(&recv->msg, 0, sizeof(recv->msg));
    // support both ipv4/6 lenght
    recv->msg.msg_namelen = sizeof(struct sockaddr_in6);

    io_recvmsg_multi(&trk->io, trk->fd, BG_ID_RECV, &recv->msg, &recv->op, received_msg);
}

int tracker_socket(struct tracker *trk)
{
    trk->fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(32141);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int ret = bind(trk->fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    return ret;
}

int tracker_tick(struct tracker *trk)
{
    int ret;

    ret = tracker_socket(trk);
    if (ret)
        return ret;

    start_receiving(trk);

    while (1)
    {
        ret = io_tick(&trk->io, trk);
        if (ret)
            return ret;
    }

    return ret;
}
