#include <stdio.h>
#include <string.h>
#include "client.h"
#include "cache.h"
#include "memory.h"
#include "assert.h"
#include "packet.h"
#include "utils.h"

struct conn
{
    int fd;
    struct sockaddr_in addr;
    struct op op;
    unsigned long connection_id;

    char send_buf[1024];
    char recv_buf[1024];
};

void sock_connected(void *ctx, struct op *op, struct io_uring_cqe *cqe);
void connect_sent(void *ctx, struct op *op, struct io_uring_cqe *cqe);
void connect_received(void *ctx, struct op *op, struct io_uring_cqe *cqe);
void announce_sent(void *ctx, struct op *op, struct io_uring_cqe *cqe);
void announce_received(void *ctx, struct op *op, struct io_uring_cqe *cqe);

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
    conn.addr.sin_addr.s_addr = inet_addr("93.158.213.92");
    conn.addr.sin_port = htons(1337);

    io_connect(&client.io, conn.fd, &conn.addr, sizeof(struct sockaddr_in), &conn.op, sock_connected);

    while (1)
        io_tick(&client.io, &client);
}

void sock_connected(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client *client = ctx;
    struct conn *conn = container_of(op, struct conn, op);
    struct connect_req *req = &conn->send_buf;

    req->header.connection_id = PROTOCOL_ID;
    req->header.action = ActionConnect;
    req->header.transaction_id = 1;
    printf("sock connected\n");

    io_send(&client->io, conn->fd, &conn->send_buf, sizeof(struct connect_req), &conn->op, connect_sent);
}

void connect_sent(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client *client = ctx;
    struct conn *conn = container_of(op, struct conn, op);
    printf("connect sent\n");

    io_recv(&client->io, conn->fd, &conn->recv_buf, sizeof(struct connect_resp), &conn->op, connect_received);
}

int parse_infohash(infohash_t *hash, char *str)
{
    unsigned char a, b;
    unsigned char *out = hash;
    for (int i = 0; i < sizeof(infohash_t); i++)
    {
        a = str[i * 2] <= '9' ? str[i * 2] - '0' : (str[i * 2] <= 'f' ? str[i * 2] - 'a' + 10 : (({ return 1; }), 0));
        b = str[i * 2 + 1] <= '9' ? str[i * 2 + 1] - '0' : (str[i * 2 + 1] <= 'f' ? str[i * 2 + 1] - 'a' + 10 : (({ return 1; }), 0));
        out[i] = a << 4 | b;
    }

    return 0;
}

void connect_received(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client *client = ctx;
    struct conn *conn = container_of(op, struct conn, op);
    struct header_resp *header_resp = &conn->recv_buf;
    struct connect_resp *resp;
    struct announce_req *req;

    assert(header_resp->transaction_id == 1);
    assert(header_resp->action == ActionConnect);
    assert(conn->connection_id == 0);
    assert(cqe->res == sizeof(struct connect_resp));

    printf("connect received\n");

    resp = &conn->recv_buf;
    // save connection id
    conn->connection_id = resp->connection_id;

    // prepare request
    req = &conn->send_buf;
    req->header.transaction_id = 2;
    req->header.connection_id = conn->connection_id;
    req->header.action = ActionAnnounce;
    req->downloaded = 0;
    req->event = EventNone;
    req->ip_address = 0;
    req->port = 6124;
    req->key = 0;
    req->left = 0;
    req->uploaded = 0;
    req->num_want = 1000;
    memset(&req->peer_id, 0, sizeof(req->peer_id));
    int ret = parse_infohash(&req->info_hash, "208b101a0f51514ecf285885a8b0f6fb1a1e4d7d");
    assert(!ret);

    io_send(&client->io, conn->fd, &conn->send_buf, sizeof(struct announce_req), &conn->op, announce_sent);
}

void announce_sent(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client *client = ctx;
    struct conn *conn = container_of(op, struct conn, op);

    printf("announce sent\n");

    io_recv(&client->io, conn->fd, &conn->recv_buf, sizeof(struct announce_resp) + sizeof(struct peer) * 1000, &conn->op, announce_received);
}

void announce_received(void *ctx, struct op *op, struct io_uring_cqe *cqe)
{
    struct client *client = ctx;
    struct conn *conn = container_of(op, struct conn, op);
    struct header_resp *header = &conn->recv_buf;
    struct announce_resp *resp;

    if (header->action == ActionError)
    {
        struct error_resp *resp = &conn->recv_buf;
        unsigned int str_len = cqe->res - sizeof(struct header_resp);

        printf("err: %.*s\n", str_len, resp->message);
        return;
    }

    assert(header->action == ActionAnnounce);
    assert(header->transaction_id == 2);
    assert(cqe->res >= sizeof(struct announce_resp));

    printf("announce received\n");

    resp = &conn->recv_buf;
    printf("leechers: %d seeders: %d interval: %d\n", ntohl(resp->leechers), ntohl(resp->seeders), ntohl(resp->interval));

    int peers = (cqe->res - sizeof(struct announce_resp)) / sizeof(struct peer);

    for (int i = 0; i < peers; i++)
    {
        unsigned char *p = &resp->peers[i].address;
        printf("%d.%d.%d.%d:%d\n", p[0], p[1], p[2], p[3], ntohs(resp->peers[i].port));
    }
}