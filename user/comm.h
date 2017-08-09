#ifndef _COMM_H
#define _COMM_H

#include "xtea.h"
#define COMM_HTTP_MAX_HOST_LENGTH 64
#define COMM_HTTP_MAX_PATH_LENGTH 128

typedef void (comm_cb_t)(uint8_t err, uint8_t* buf, uint32_t len);

typedef struct comm_info {
  uint8_t protocol;
  uint32_t send_buffer_size;
  uint32_t recv_buffer_size;
  union {
    const struct hxdt_info* hxdt;
  };
} comm_info_t;

typedef struct hxdt_info {
  char hostname[255];
  char path[255];
  uint16_t port;
  uint8_t encrypt_key[XTEA_KEY_SIZE];
  uint8_t auth_key[XTEA_KEY_SIZE];
  uint8_t auth_iv[XTEA_BLOCK_SIZE];
} hxdt_info_t;

// the following are supposed to be private...
typedef struct comm_state* comm_state_t;
typedef struct comm_buffer comm_buffer_t;

enum {
  COMM_OK = 0,
  COMM_UNSUPPORTED_PROTOCOL,
  COMM_INVALID_ARGS,
  COMM_BUFFER_OVERFLOW,
  COMM_MALLOC_ERROR,
  COMM_CLIENT_GENERIC_ERROR,
  COMM_SERVER_GENERIC_ERROR,
  COMM_NOT_READY
};

enum {
  COMM_PROTOCOL_HXDT = 0,
  COMM_PROTOCOL_TXDT
};

uint8_t comm_create   (comm_state_t* state_ptr, comm_info_t info, comm_cb_t* cb);
uint8_t comm_write    (comm_state_t state, void* buf, uint32_t size);
uint8_t comm_writef   (comm_state_t state, char* fmt, ...);
void    comm_send     (comm_state_t state);
uint8_t comm_destroy  (comm_state_t state);

#endif
