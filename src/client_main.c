#include <stdio.h>
#include "client.h"
#include "cache.h"
#include "memory.h"
#include "assert.h"
#include "packet.h"
#include "utils.h"

void packed_received(void *ctx, struct op *op, struct io_uring_cqe *cqe);
void sock_connected(void *ctx, struct op *op, struct io_uring_cqe *cqe);
void sock_connected(void *ctx, struct op *op, struct io_uring_cqe *cqe);
struct conn;

struct conn
{
    int fd;
    struct sockaddr_in addr;
    struct op op;
    unsigned long connection_id;

    char send_buf[1024];
    char recv_buf[1024];
};

int main()
{
    int ret;

    // struct mm_pool mm;
    // struct cache send_op_cache;
    // void *buf;
    // int pages = 1024;

    // ret = alloc_locked_pages(pages, &buf);
    // if (ret)
    //     return ret;

    // mm_pool_init(&mm, buf, pages);
    // cache_init(&send_op_cache, sizeof(struct op_send));

    struct client client;

    ret = client_init(&client);
    if (ret)
        return ret;

    // struct op_send *send = mm_pool_cache_get(&mm, &send_op_cache);
    // assert(send);

    struct conn conn;
    conn.connection_id = 0;
    conn.fd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(conn.fd != -1);

    conn.addr.sin_family = AF_INET;
    conn.addr.sin_addr.s_addr = inet_addr("23.157.120.14");
    conn.addr.sin_port = htons(6969);

    io_connect(&client.io, conn.fd, &conn.addr, sizeof(struct sockaddr_in), &conn.op, sock_connected);

    while (1)
        io_tick(&client.io, &client);
}

void packed_received(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client* client = ctx;
    struct conn *conn = container_of(op, struct conn, op);
    struct header_resp *resp = &conn->recv_buf;
    assert(resp->transaction_id == 1234);
    printf("%d\n", resp->action);

    switch (resp->action)
    {
    case ActionConnect:
        assert(cqe->res == sizeof(struct connect_resp));
        if (conn->connection_id == 0)
        {
            struct connect_resp *resp = &conn->recv_buf;
            conn->connection_id = resp->connection_id;
            printf("saved connection id\n");
        }
        else
        {
        }
        break;

    default:
        break;
    }
}

void connect_sent(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client* client = ctx;
    struct conn *conn = container_of(op, struct conn, op);

    io_recv(&client->io, conn->fd, &conn->recv_buf, sizeof(struct connect_resp), &conn->op, packed_received);
}

void sock_connected(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client* client = ctx;
    struct conn *conn = container_of(op, struct conn, op);
    struct connect_req *req = &conn->send_buf;

    req->header.connection_id = PROTOCOL_ID;
    req->header.action = ActionConnect;
    req->header.transaction_id = 1234;

    io_send(&client->io, conn->fd, &conn->send_buf, sizeof(struct connect_req), &conn->op, connect_sent);
}

