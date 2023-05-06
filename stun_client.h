#ifndef __STUN_CLIENT_H__
#define __STUN_CLIENT_H__

#include "stdio.h"
#include "stddef.h"
#include "esp_err.h"

typedef struct {
    uint16_t type;
    uint16_t length;
    uint32_t magic;
    uint32_t payload[3];
} stun_msg_t;

typedef struct {
    char        *stun_server;
    uint16_t    stun_port;
    uint16_t    local_port;
} stun_context;

#define MAGIC_COOKIE            0x2112A442
#define MAPPED_ADDRESS          0x0001
#define XOR_MAPPED_ADDRESS      0x0020
#define SOFTWARE                0x8022

static char list_server[][100] ={
    "stun1.l.google.com:19302",
    "stun2.l.google.com:19302",
    "stun3.l.google.com:19302",
    "stun4.l.google.com:19302",
};
static short list_server_count = sizeof(list_server)/sizeof(list_server[0]);

esp_err_t stun_client();

#endif
