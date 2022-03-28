#ifndef MAOCK_CONFIG_H
#define MAOCK_CONFIG_H
#include <stdint.h>

struct Maock_Config
{
    uint32_t nghttp2_max_concurrent_streams = 256;
};

extern Maock_Config maock_config;

#endif
