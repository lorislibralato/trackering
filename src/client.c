#include "client.h"

int client_init(struct client *client)
{
    int ret;

    struct io_options ops = {.attach_wq = -1, .entries = 1 << 10, .cqe_order = 0, .sq_cpu = -1, .sq_idle = 0, .use_direct_files = 0};
    ret = io_init(&client->io, &ops);
    if (ret)
        return ret;
}