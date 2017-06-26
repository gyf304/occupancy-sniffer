#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "comm.h"
#include "config.h"

#define xstr(s) str(s)
#define str(s) #s

#define HTTP_HEADERS \
"User-Agent: ESP8266\r\n"\
"Content-Type: application/octet-stream\r\n"\
"Connection: Close\r\n"

struct http_state {
  char* hostname;
  char* buffer;
  uint32_t buffer_size;
  uint16_t port;
  uint32_t len;
  uint32_t content_length_pos;
  uint32_t request_len;
} __attribute__ ((aligned (4)));

struct comm_state {
  uint8_t protocol;
} __attribute__ ((aligned (4)));

static inline bool http_write(struct http_state* state, void* str, uint32_t len);
static inline bool http_write_str(struct http_state* state, char* str);
static uint8_t http_init( struct http_state* state, 
                          const struct http_info* info, 
                          const uint32_t buflen);
static uint8_t http_add (struct http_state* state, uint8_t* buf, uint32_t size);
static uint8_t http_finalize (struct http_state* state);
#define http_write_str_lit(buf, s) http_write((buf), (s), sizeof(s)-1)

uint8_t ICACHE_FLASH_ATTR
comm_create(comm_state_t* state_ptr, const comm_info_t* info, uint32_t buflen)
{
  switch (info->protocol) {
    case COMM_PROTOCOL_HTTP_XTEA_RPC: {
      uint32_t size = sizeof(struct comm_state) + sizeof(struct http_state);
      comm_state_t state = os_malloc(size);
      if (!state) return COMM_MALLOC_ERROR;
      os_memset(state, 0, size);
      state->protocol = info->protocol;
      uint8_t err = http_init((struct http_state*)(((uint8_t*)state) + sizeof(struct comm_state)), &info->http, buflen);
      if (err == COMM_OK) *state_ptr = state; 
      else os_free(state);
      return err;
      break;
    }
    default:
      return COMM_UNSUPPORTED_PROTOCOL;
      break;
  }
}

uint8_t ICACHE_FLASH_ATTR
comm_add(comm_state_t state, uint8_t* buf, uint32_t buflen)
{
  switch (state->protocol) {
    case COMM_PROTOCOL_HTTP_XTEA_RPC:
      return http_add((struct http_state*)(((uint8_t*)state) + sizeof(struct comm_state)), buf, buflen);
      break;
    default:
      return COMM_GENERIC_ERROR;
      break;
  }
}

uint8_t ICACHE_FLASH_ATTR
comm_destroy(comm_state_t state)
{
  os_free(state);
  switch (state->protocol) {
    case COMM_PROTOCOL_HTTP_XTEA_RPC:
      return http_finalize((struct http_state*)(((uint8_t*)state) + sizeof(struct comm_state)));
      break;
    default:
      return COMM_GENERIC_ERROR;
      break;
  }
  return true;
}

static inline bool http_write(struct http_state* state, void* str, uint32_t len)
{
  if ((len + state->len + 1) >= state->buffer_size) return false;
  uint8_t* buf = (uint8_t*)state->buffer;
  os_memcpy(&buf[state->len], str, len);
  state->len += len;
  return true;
}

static inline bool http_write_str(struct http_state* state, char* str)
{
  uint32_t len = os_strlen(str);
  return http_write(state, (uint8_t*)str, len);
}

static uint8_t ICACHE_FLASH_ATTR
http_init(struct http_state* state, const struct http_info* info, const uint32_t buflen)
{
  state->buffer = os_malloc(buflen);
  if (state->buffer == NULL) return COMM_MALLOC_ERROR;
  os_memset(state->buffer, 0, buflen);
  uint32_t hostname_len = os_strlen(info->hostname) + 1;
  state->hostname = os_malloc(hostname_len);
  if (state->hostname == NULL) return COMM_MALLOC_ERROR;
  os_memcpy(state->hostname, info->hostname, hostname_len);
  state->port = info->port;
  state->len = 0;
  state->buffer_size = buflen;
  if (!http_write_str_lit(state, "POST ")) return COMM_BUFFER_OVERFLOW;
  if (!http_write_str(state, info->path)) return COMM_BUFFER_OVERFLOW;
  if (!http_write_str_lit(state, " HTTP/1.1\r\n")) return COMM_BUFFER_OVERFLOW;
  if (!http_write_str_lit(state, HTTP_HEADERS)) return COMM_BUFFER_OVERFLOW;
  if (!http_write_str_lit(state, "Host: ")) return COMM_BUFFER_OVERFLOW;
  if (!http_write_str(state, info->hostname)) return COMM_BUFFER_OVERFLOW;
  if (state->port != 80) {
    char portstr[8];
    os_memset(&portstr[0], 0, 8);
    os_sprintf(&portstr[0], ":%d", state->port);
    if (!http_write_str(state, &portstr[0])) return COMM_BUFFER_OVERFLOW;
  }
  if (!http_write_str_lit(state, "\r\nContent-Length: ")) return COMM_BUFFER_OVERFLOW;
  state->content_length_pos = state->len;
  if (!http_write(state, "            ", 12-state->len%4)) return COMM_BUFFER_OVERFLOW;
  if (!http_write_str_lit(state, "\r\n\r\n")) return COMM_BUFFER_OVERFLOW;
  state->request_len = state->len;
  // ver/mode, raw_length, payload_length
  if (!http_write(state, (uint8_t[]){0,0,0,0,0,0,0,0,0,0,0,0}, 12)) return COMM_BUFFER_OVERFLOW;
  #ifdef DEBUG
    os_printf("HTTP INIT: \r\n");
    os_printf("%s", state->buffer);
    os_printf("PTR: 0x%08x\n", (uint32_t)&state->buffer[state->len]);
  #endif
  return COMM_OK;
}

static uint8_t ICACHE_FLASH_ATTR
http_finalize (struct http_state* state)
{
  os_free(state->hostname);
  os_free(state->buffer);
  return COMM_OK;
}

static uint8_t ICACHE_FLASH_ATTR
http_add (struct http_state* state, uint8_t* buf, uint32_t size)
{
  if (http_write(state, buf, size)) return COMM_OK;
  return COMM_BUFFER_OVERFLOW;
}
