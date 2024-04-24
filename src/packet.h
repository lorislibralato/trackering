#ifndef PACKET_H
#define PACKET_H

enum __attribute__((__packed__)) action
{
    ActionConnect = __bswap_constant_32(0),
    ActionAnnounce = __bswap_constant_32(1),
    ActionScrape = __bswap_constant_32(2), 
    ActionError = __bswap_constant_32(3),

    Max = INT32_MAX,
};
static_assert(sizeof(enum action) == 4, "invalid byte size");
static_assert(ActionConnect == 0b0, "invalid connect bits");

typedef char infohash_t[20];
static_assert(sizeof(infohash_t) == 20, "invalid byte size");

struct __attribute__((__packed__)) header_req
{
    unsigned long connection_id;
    unsigned int action;
    unsigned int transaction_id;
};
static_assert(sizeof(struct header_req) == 16, "invalid byte size");

struct __attribute__((__packed__)) connect_req
{
    struct header_req header;
};
static_assert(sizeof(struct connect_req) == 16, "invalid byte size");
#define PROTOCOL_ID (__bswap_constant_64(0x41727101980UL))

enum __attribute__((__packed__)) announce_event
{
    EventNone = __bswap_constant_32(0),
    EventCompleted = __bswap_constant_32(1),
    EventStarted = __bswap_constant_32(2),
    EventStopped = __bswap_constant_32(3),

    MAX = INT32_MAX,
};
static_assert(sizeof(enum announce_event) == 4, "invalid byte size");

struct __attribute__((__packed__)) announce_req
{
    struct header_req header;
    infohash_t info_hash;
    infohash_t peer_id;
    unsigned long downloaded;
    unsigned long left;
    unsigned long uploaded;
    unsigned int event;
    unsigned int ip_address;
    unsigned int key;
    unsigned int num_want;
    unsigned short port;
};
static_assert(sizeof(struct announce_req) == 98, "invalid byte size");

struct __attribute__((__packed__)) scrape_req
{
    struct header_req header;
    infohash_t torrents[0];
};
static_assert(sizeof(struct scrape_req) == 16, "invalid byte size");

struct __attribute__((__packed__)) header_resp
{
    unsigned int action;
    unsigned int transaction_id;
};
static_assert(sizeof(struct header_resp) == 8, "invalid byte size");

struct __attribute__((__packed__)) connect_resp
{
    struct header_resp header;
    unsigned long connection_id;
};
static_assert(sizeof(struct connect_resp) == 16, "invalid byte size");

struct __attribute__((__packed__)) peer
{
    unsigned int address;
    unsigned short port;
};
static_assert(sizeof(struct peer) == 6, "invalid byte size");

struct __attribute__((__packed__)) announce_resp
{
    struct header_resp header;
    unsigned int interval;
    unsigned int leechers;
    unsigned int seeders;
    struct peer peer[0];
};
static_assert(sizeof(struct announce_resp) == 20, "invalid byte size");

struct __attribute__((__packed__)) torrent_info
{
    unsigned int seeders;
    unsigned int completed;
    unsigned int leechers;
};
static_assert(sizeof(struct torrent_info) == 12, "invalid byte size");

struct __attribute__((__packed__)) scrape_resp
{
    struct header_resp header;
    struct torrent_info torrents[0];
};
static_assert(sizeof(struct scrape_resp) == 8, "invalid byte size");

struct __attribute__((__packed__)) error_resp
{
    struct header_resp header;
    char message[0];
};
static_assert(sizeof(struct error_resp) == 8, "invalid byte size");

#endif