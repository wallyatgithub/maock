#ifndef MAOCK_CONFIG_H
#define MAOCK_CONFIG_H
#include <stdint.h>

struct Maock_Config
{
    uint32_t nghttp2_max_concurrent_streams = 256;
    uint64_t skt_recv_buffer_size = 4 * 1024 * 1024;
    uint64_t skt_send_buffer_size = 4 * 1024 * 1024;
};

extern Maock_Config maock_config;

#endif
