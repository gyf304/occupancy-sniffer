#include <stddef.h>
#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "comm.h"
#include "config.h"
#include "endian.h"
#include "espconn.h"
#include "comm_hxdt.h"

static comm_state_t states[8];
static uint8_t cur_state = 0;
static uint8_t end_state = 0;

uint8_t ICACHE_FLASH_ATTR
comm_create(comm_state_t* state_ptr, comm_info_t info, uint32_t max_size, comm_cb_t* cb)
{
  comm_state_t state = os_malloc(sizeof(struct comm_state));
  state->protocol = info.protocol;
  state->info = info;
  state->buffer = os_malloc(max_size);
  state->size = 0;
  state->max_size = max_size;
  state->cb = cb;
  *state_ptr = state;
  return 0;
}

uint8_t ICACHE_FLASH_ATTR
comm_write(comm_state_t state, void* buf, uint32_t buflen)
{
  if (state->max_size - buflen <= buflen) return COMM_BUFFER_OVERFLOW;
  os_memcpy(&state->buffer[state->size], buf, buflen);
  state->size += buflen;
  return 0;
}

uint8_t ICACHE_FLASH_ATTR
comm_destroy(comm_state_t state)
{
  if(state->buffer) os_free(state->buffer);
  os_free(state);
  return 0;
}

void ICACHE_FLASH_ATTR
comm_send(comm_state_t state)
{
  switch(state->protocol) {
    case COMM_PROTOCOL_HXDT:
      comm_send_hxdt(state);
      break;
  }
  return;
}