#ifndef CLIENT_H
#define CLIENT_H

#include "io.h"

struct client {
    struct io io;
};

int client_init(struct client*);

#endif // CLIENT_H