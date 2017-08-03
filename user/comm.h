#ifndef _COMM_H
#define _COMM_H

#define COMM_HTTP_MAX_HOST_LENGTH 64
#define COMM_HTTP_MAX_PATH_LENGTH 128

typedef struct comm_cb_info {
  uint8_t  err;
  uint8_t* buf;
  uint32_t len;
} comm_cb_info_t;

typedef void (comm_cb_t)(comm_cb_info_t*);

typedef struct hxdt_info {
  char* hostname;
  char* path;
  uint16_t port;
  uint8_t* encrypt_key;
  uint8_t* auth_key;
  uint8_t* auth_iv;
} hxdt_info_t;

typedef struct comm_info {
  uint8_t protocol;
  union {
    hxdt_info_t hxdt;
  };
} comm_info_t;

// the following are supposed to be private...
typedef struct comm_state* comm_state_t;

enum {
  COMM_OK = 0,
  COMM_UNSUPPORTED_PROTOCOL,
  COMM_INVALID_ARGS,
  COMM_GENERIC_ERROR,
  COMM_BUFFER_OVERFLOW,
  COMM_MALLOC_ERROR
};

enum {
  COMM_PROTOCOL_HXDT = 0,
  COMM_PROTOCOL_TXDT
};

enum {
  COMM_CB_DATA = 0,
  COMM_CB_ERROR
};


uint8_t comm_create   (comm_state_t* state_ptr, const comm_info_t* info, uint32_t buflen);
uint8_t comm_write    (comm_state_t state, uint8_t* buf, uint32_t size);
uint8_t comm_writef   (comm_state_t state, char* fmt, ...);
uint8_t comm_send     (comm_state_t state);
uint8_t comm_destroy  (comm_state_t state);

#endif
