#ifndef _COMM_INTERNAL_H
#define _COMM_INTERNAL_H
#include "comm.h"

struct comm_buffer {
  uint8_t* buffer;
  uint32_t len;
  uint32_t size;
};

struct comm_state {
  uint8_t protocol;
  struct comm_buffer send_buffer;
  struct comm_buffer recv_buffer;
  comm_cb_t* cb;
  union {
    struct hxdt_info* hxdt;
    void* dummy;
  } info;
} __attribute__ ((aligned (4)));

uint8_t comm_buffer_write(struct comm_buffer* cbuf, void* buf, uint32_t len);

#endif