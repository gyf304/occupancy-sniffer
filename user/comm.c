#include <stddef.h>
#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "comm_internal.h"
#include "comm_hxdt.h"

static comm_state_t states[8];
static uint8_t cur_state = 0;
static uint8_t end_state = 0;

uint8_t ICACHE_FLASH_ATTR
comm_create(comm_state_t* state_ptr, comm_info_t info, comm_cb_t* cb)
{
  comm_state_t state = os_zalloc(sizeof(struct comm_state));
  if (state == NULL) {
    return COMM_MALLOC_ERROR;
  }
  state->protocol = info.protocol;
  state->send_buffer.buffer = os_malloc(info.send_buffer_size);
  if (state->send_buffer.buffer == NULL) {
    comm_destroy(state);
    return COMM_MALLOC_ERROR;
  }
  state->send_buffer.len = 0;
  state->send_buffer.size = info.send_buffer_size;

  state->recv_buffer.buffer = os_malloc(info.recv_buffer_size);
  if (state->recv_buffer.buffer == NULL) {
    comm_destroy(state);
    return COMM_MALLOC_ERROR;
  }
  state->recv_buffer.len = 0;
  state->recv_buffer.size = info.recv_buffer_size;

  switch (info.protocol) {
    case COMM_PROTOCOL_HXDT:
      state->info.hxdt = os_malloc(sizeof(struct hxdt_info));
      if (state->info.hxdt == NULL) {
        comm_destroy(state);
        return COMM_MALLOC_ERROR;
      }
      os_memcpy(state->info.hxdt, info.hxdt, sizeof(struct hxdt_info));
      break;
    default:
      comm_destroy(state);
      return COMM_UNSUPPORTED_PROTOCOL;
  }

  state->cb = cb;
  *state_ptr = state;
  return 0;
}

uint8_t ICACHE_FLASH_ATTR
comm_buffer_write(struct comm_buffer* cbuf, void* buf, uint32_t len)
{
  if (cbuf->size - cbuf->len < len) return COMM_BUFFER_OVERFLOW;
  os_memcpy(&cbuf->buffer[cbuf->len], buf, len);
  cbuf->len += len;
  return COMM_OK;
}

uint8_t ICACHE_FLASH_ATTR
comm_write(comm_state_t state, void* buf, uint32_t buflen)
{
  return comm_buffer_write(&state->send_buffer, buf, buflen);
}

uint8_t ICACHE_FLASH_ATTR
comm_destroy(comm_state_t state)
{
  if(state->send_buffer.buffer) os_free(state->send_buffer.buffer);
  if(state->recv_buffer.buffer) os_free(state->recv_buffer.buffer);
  if(state->info.dummy) os_free(state->info.dummy);
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